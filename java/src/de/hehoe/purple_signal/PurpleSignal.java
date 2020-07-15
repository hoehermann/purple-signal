package de.hehoe.purple_signal;

import java.security.Security;
import org.bouncycastle.jce.provider.BouncyCastleProvider;

import java.io.File;
import java.io.IOException;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import org.asamk.signal.manager.AttachmentInvalidException;
import org.asamk.signal.manager.GroupNotFoundException;
import org.asamk.signal.manager.Manager;
import org.asamk.signal.manager.Manager.ReceiveMessageHandler;
import org.asamk.signal.manager.NotAGroupMemberException;
import org.asamk.signal.manager.ProvisioningManager;
import org.asamk.signal.manager.ServiceConfig;
import org.asamk.signal.manager.UserAlreadyExists;
import org.asamk.signal.storage.SignalAccount;
import org.asamk.signal.util.SecurityProvider;
import org.asamk.signal.util.Util;
import org.whispersystems.libsignal.InvalidKeyException;
import org.whispersystems.signalservice.api.messages.SignalServiceAttachment;
import org.whispersystems.signalservice.api.messages.SignalServiceContent;
import org.whispersystems.signalservice.api.messages.SignalServiceDataMessage;
import org.whispersystems.signalservice.api.messages.SignalServiceEnvelope;
import org.whispersystems.signalservice.api.messages.SignalServiceGroup;
import org.whispersystems.signalservice.api.messages.SignalServiceGroup.Type;
import org.whispersystems.signalservice.api.messages.calls.SignalServiceCallMessage;
import org.whispersystems.signalservice.api.messages.SignalServiceGroupContext;
import org.whispersystems.signalservice.api.messages.multidevice.ReadMessage;
import org.whispersystems.signalservice.api.messages.multidevice.SentTranscriptMessage;
import org.whispersystems.signalservice.api.messages.multidevice.SignalServiceSyncMessage;
import org.whispersystems.signalservice.api.push.SignalServiceAddress;
import org.whispersystems.signalservice.api.push.exceptions.EncapsulatedExceptions;
import org.whispersystems.signalservice.api.util.InvalidNumberException;
import org.whispersystems.signalservice.internal.configuration.SignalServiceConfiguration;
import org.whispersystems.util.Base64;
import org.asamk.signal.util.GroupIdFormatException;
import org.asamk.signal.util.IOUtils;

public class PurpleSignal implements ReceiveMessageHandler, Runnable {

    // stolen from signal-cli/src/main/java/org/asamk/signal/Main.java
    /**
     * Uses $HOME/.local/share/signal-cli if it exists, or if none of the legacy
     * directories exist: - $HOME/.config/signal - $HOME/.config/textsecure
     *
     * @return the data directory to be used by signal-cli.
     */
    private static String getDefaultDataPath() {
        String dataPath = IOUtils.getDataHomeDir() + "/signal-cli";
        if (new File(dataPath).exists()) {
            return dataPath;
        }
        String legacySettingsPath = System.getProperty("user.home") + "/.config/signal";
        if (new File(legacySettingsPath).exists()) {
            return legacySettingsPath;
        }
        legacySettingsPath = System.getProperty("user.home") + "/.config/textsecure";
        if (new File(legacySettingsPath).exists()) {
            return legacySettingsPath;
        }
        return dataPath;
    }

    private Manager manager = null;
    private ProvisioningManager provisioningManager = null;
    private long connection = 0;
    private boolean keepReceiving = false;
    private String username = null;

    public PurpleSignal(long connection, String username, String settingsDir) throws IOException, TimeoutException, InvalidKeyException, UserAlreadyExists {
        final String USER_AGENT = "purple-signal";
        final SignalServiceConfiguration serviceConfiguration = ServiceConfig.createDefaultServiceConfiguration(USER_AGENT);

        this.connection = connection;
        this.username = username;
        this.keepReceiving = false;

        // stolen from signald/src/main/java/io/finn/signald/Main.java
        // Workaround for BKS truststore
        Security.insertProviderAt(new SecurityProvider(), 1);
        Security.addProvider(new BouncyCastleProvider());
        
        String settingsPath = getDefaultDataPath();
        if (!settingsDir.isEmpty()) {
        	settingsPath = settingsDir;
        }
        logNatively(DEBUG_LEVEL_INFO, "Using settings path " + settingsPath);
        String dataPath = settingsPath + "/data";
        if (SignalAccount.userExists(dataPath, this.username)) {
        	this.manager = Manager.init(this.username, settingsPath, serviceConfiguration, USER_AGENT);
            if (!this.manager.isRegistered()) {
            	throw new IllegalStateException("User is not registered but exists at "+dataPath+". Either link successfully or register and verify with signal-cli.");
            }
        	this.manager.checkAccountState();
            logNatively(DEBUG_LEVEL_INFO, "Using registered user " + manager.getUsername());
        } else {
            logNatively(DEBUG_LEVEL_INFO, "User does not exist. Asking to link…");
            this.provisioningManager = new ProvisioningManager(settingsPath, serviceConfiguration, USER_AGENT);
            final String deviceLinkUri = this.provisioningManager.getDeviceLinkUri();
            handleMessageNatively(
        		this.connection, 
        		"link",
        		this.username,
        		"Please use this code to link this Pidgin account (use a QR encoder for linking with real phones):<br/>"+deviceLinkUri,
        		0,
        		PURPLE_MESSAGE_NICK
            );
        }
    }

    public void run() {
    	if (this.provisioningManager != null) {
            String linkedUsername = null;
			try {
				linkedUsername = this.provisioningManager.finishDeviceLink("purple-signal");
			} catch (IOException | InvalidKeyException | TimeoutException | UserAlreadyExists e) {
				handleErrorNatively(this.connection, "Unable to finish device link: " + e.getMessage());
				e.printStackTrace();
			}
			if (linkedUsername.equals(this.username)) {
				handleErrorNatively(this.connection, String.format(
							"Configured username %s does not match linked number %s.", 
							this.username, linkedUsername
				));
			} else {
				handleErrorNatively(this.connection, "Reconnect needed.");
			}
    	} else {
            boolean ignoreAttachments = true;
	        boolean returnOnTimeout = true; // it looks like setting this to false means "listen for new messages forever".
	        // There seems to be a non-daemon thread to be involved somewhere as the Java VM
	        // will not ever shut down.
	        long timeout = 60; // Seconds to wait for an incoming message. After the timeout occurred, a re-connect happens silently.
	        // TODO: Find out how this affects what.
	        try {
	            while (this.keepReceiving) {
	                this.manager.receiveMessages(
                		(long) (timeout * 1000), 
                		TimeUnit.MILLISECONDS, 
                		returnOnTimeout,
	                    ignoreAttachments, 
	                    this
	                );
	            }
	        } catch (Exception e) {
	            handleErrorNatively(this.connection, "Exception while waiting for message: " + e.getMessage());
	        } catch (Throwable t) {
	            handleErrorNatively(this.connection, "Unhandled exception while waiting for message.");
	            t.printStackTrace();
	        }
    	}
        logNatively(DEBUG_LEVEL_INFO, "RECEIVING DONE");
    }

    public void startReceiving() {
        this.keepReceiving = true;
        Thread t = new Thread(this);
        t.setName("Receiver");
        t.setDaemon(true);
        t.start();
    }

    public void stopReceiving() {
        this.keepReceiving = false;
        if (this.manager != null) {
            try {
                this.manager.close();
            } catch (IOException e) {
                // I really don't care
                e.printStackTrace();
            }
        }
    }

    @Override
    public void handleMessage(SignalServiceEnvelope envelope, SignalServiceContent content, Throwable exception) {
        logNatively(DEBUG_LEVEL_INFO, "RECEIVED SOMETHING!");
        // stolen from signald/src/main/java/io/finn/signald/MessageReceiver.java and
        // signal-cli/src/main/java/org/asamk/signal/JsonMessageEnvelope.java and
        // signal-cli/src/main/java/org/asamk/signal/ReceiveMessageHandler.java
        if (exception != null) {
            handleErrorNatively(this.connection, "Exception while handling message: " + exception.getMessage());
        } else if (envelope == null) {
        	handleErrorNatively(this.connection, "Handling null envelope."); // this should never happen
        } else {
        	printEnvelope(envelope);
            String source = envelope.getSourceAddress().getNumber().orNull();
            if (source == null) {
            	logNatively(DEBUG_LEVEL_INFO, "Source is null. Ignoring message.");
            } else if (envelope.isReceipt()) {
            	logNatively(DEBUG_LEVEL_INFO, "Ignoring receipt.");
            } else if (content == null) {
            	logNatively(DEBUG_LEVEL_INFO, "Failed to decrypt incoming message. Ignoring message.");
                //handleErrorNatively(this.connection, "Failed to decrypt incoming message.");
            } else {
            	long timestamp = envelope.getTimestamp();
            	printSignalServiceContent(content);
                if (content.getDataMessage().isPresent()) {
                    handleDataMessage(content, source);
                } else if (content.getSyncMessage().isPresent()) {
                    handleSyncMessage(content, source);
                } else if (content.getTypingMessage().isPresent()) {
                	logNatively(DEBUG_LEVEL_INFO, "Received typing message for "+source+". Ignoring.");
                } else if (content.getReceiptMessage().isPresent()) {
					handleMessageNatively(this.connection, source, source, "[Message was received.]", timestamp, PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NO_LOG);
                } else {
                	handleMessageNatively(this.connection, source, source, "[Received message of unknown type.]", timestamp, PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NO_LOG);
                }
                // TODO: support all message types
            }
        }

    }

	private void printEnvelope(SignalServiceEnvelope envelope) {
		System.out.println(printPrefix+"SignalServiceEnvelope");
		long serverTimestamp = envelope.getServerTimestamp();
		System.out.println(printPrefix+"  ServerTimestamp: "+serverTimestamp);
		if (envelope.hasSource()) {
			SignalServiceAddress sourceAddress = envelope.getSourceAddress();
			System.out.println(printPrefix+"  SourceAddress:");
			if (sourceAddress.getNumber().isPresent()) {
				String number = sourceAddress.getNumber().get();
				System.out.println(printPrefix+"    Number: "+number);
			}
			//envelope.getSourceDevice();
			//envelope.getSourceE164();
			//envelope.getSourceIdentifier();
			//envelope.getSourceUuid();
		}
		long timestamp = envelope.getTimestamp();
		System.out.println(printPrefix+"  Timestamp: "+timestamp);
		if (envelope.isPreKeySignalMessage()) {
			System.out.println(printPrefix+"  isPreKeySignalMessage");
		}
		if (envelope.isReceipt()) {
			System.out.println(printPrefix+"  isReceipt");
		}
		if (envelope.isSignalMessage()) {
			System.out.println(printPrefix+"  isSignalMessage");
		}
		if (envelope.isUnidentifiedSender()) {
			System.out.println(printPrefix+"  isUnidentifiedSender");
		}
	}
	
	final String printPrefix = "[signal] ";

	private void printSignalServiceContent(SignalServiceContent content) {
		System.out.println(printPrefix+"SignalServiceContent");
		if (content.getCallMessage().isPresent()) {
			SignalServiceCallMessage callMessage = content.getCallMessage().get();
			System.out.println(printPrefix+"  CallMessage: "+callMessage.toString());
		}
		if (content.getDataMessage().isPresent()) {
			SignalServiceDataMessage dataMessage = content.getDataMessage().get();
			System.out.println(printPrefix+"  DataMessage");
			if (dataMessage.getAttachments().isPresent()) {
				List<SignalServiceAttachment> attachments = dataMessage.getAttachments().get();
				System.out.println(printPrefix+"    Attachments.size(): "+attachments.size());
			}
			if (dataMessage.getBody().isPresent()) {
				String body = dataMessage.getBody().get();
				System.out.println(printPrefix+"    Body: \""+body+"\"");
			}
			if (dataMessage.getGroupContext().isPresent()) {
				SignalServiceGroupContext groupContext = dataMessage.getGroupContext().get();
				printGroupContext(groupContext, printPrefix+"    ");
			}
		}
		if (content.getReceiptMessage().isPresent()) {
			//SignalServiceReceiptMessage receiptMessage = content.getReceiptMessage().get();
			System.out.println(printPrefix+"  ReceiptMessage: TODO");
		}
		long serverTimestamp = content.getServerTimestamp();
		System.out.println(printPrefix+"  serverTimestamp: "+serverTimestamp);
		if (content.getSyncMessage().isPresent()) {
			System.out.println(printPrefix+"  SyncMessage");
			SignalServiceSyncMessage syncMessage = content.getSyncMessage().get();
			if (syncMessage.getBlockedList().isPresent()) {
				System.out.println(printPrefix+"    BlockedList: TODO");
			};
			if (syncMessage.getConfiguration().isPresent()) {
				System.out.println(printPrefix+"    Configuration: TODO");
			};
			if (syncMessage.getContacts().isPresent()) {
				System.out.println(printPrefix+"    Contacts: TODO");
			};
			if (syncMessage.getFetchType().isPresent()) {
				System.out.println(printPrefix+"    FetchType: TODO");
			};
			if (syncMessage.getGroups().isPresent()) {
				System.out.println(printPrefix+"    Groups: TODO");
			};
			if (syncMessage.getKeys().isPresent()) {
				System.out.println(printPrefix+"    Keys: TODO");
			};
			if (syncMessage.getMessageRequestResponse().isPresent()) {
				System.out.println(printPrefix+"    MessageRequestResponse: TODO");
			};
			if (syncMessage.getRead().isPresent()) {
				System.out.println(printPrefix+"    Read:");
				List<ReadMessage> read = syncMessage.getRead().get();
				read.forEach((readMessage) -> {
					System.out.println(printPrefix+"      "+readMessage.toString());
				});
			};
			if (syncMessage.getRequest().isPresent()) {
				System.out.println(printPrefix+"    Request: TODO");
			};
			if (syncMessage.getSent().isPresent()) {
				System.out.println(printPrefix+"    Sent:");
				SentTranscriptMessage sent = syncMessage.getSent().get();
				if (sent.getDestination().isPresent()) {
					System.out.println(printPrefix+"      Destination:");
					SignalServiceAddress destination = sent.getDestination().get();
					String identifier = destination.getIdentifier();
					System.out.println(printPrefix+"        Identifier: "+identifier);
					if (destination.getNumber().isPresent()) {
						String number = destination.getNumber().get();
						System.out.println(printPrefix+"        Number: "+number);
					}
					if (destination.getRelay().isPresent()) {
						String relay = destination.getRelay().get();
						System.out.println(printPrefix+"        Relay: "+relay);
					}
				}
				long expirationStartTimestamp = sent.getExpirationStartTimestamp();
				if (expirationStartTimestamp > 0) {
					System.out.println(printPrefix+"      ExpirationStartTimestamp:"+expirationStartTimestamp);
					
				}
				SignalServiceDataMessage message = sent.getMessage();
				System.out.println(printPrefix+"      Message:");
				if (message.getAttachments().isPresent()) {
					System.out.println("        Attachments: TODO");	
				}
				if (message.getBody().isPresent()) {
					String body = message.getBody().get();
					System.out.println(printPrefix+"        Body: \""+body+"\"");
				}
				int expiresInSeconds = message.getExpiresInSeconds();
				if (expiresInSeconds > 0) {
					System.out.println(printPrefix+"        ExpiresInSeconds: "+expiresInSeconds);
				}
				if (message.getGroupContext().isPresent()) {
					SignalServiceGroupContext groupContext = message.getGroupContext().get();
					printGroupContext(groupContext, printPrefix+"        ");
				}
				System.out.println(printPrefix+"        Recipients: ");
				sent.getRecipients().forEach(serviceAdress -> {
					System.out.println(printPrefix+"          "+serviceAdress.getNumber().get());
				});
				System.out.println(printPrefix+"        timestamp: "+sent.getTimestamp());
				if (sent.isRecipientUpdate()) {
					System.out.println(printPrefix+"        isRecipientUpdate");
				}
			};
			if (syncMessage.getStickerPackOperations().isPresent()) {
				System.out.println(printPrefix+"    StickerPackOperations: TODO");
			};
			if (syncMessage.getVerified().isPresent()) {
				System.out.println(printPrefix+"    Verified: TODO");
			};
			if (syncMessage.getViewOnceOpen().isPresent()) {
				System.out.println(printPrefix+"    ViewOnceOpen: TODO");
			};
		}
		long timestamp = content.getTimestamp();
		System.out.println(printPrefix+"  timestamp: "+timestamp);
		if (content.getTypingMessage().isPresent()) {
			//SignalServiceTypingMessage typingMessage = content.getTypingMessage().get();
			System.out.println(printPrefix+"  TypingMessage: TODO");
		}
	}

	private void printGroupContext(SignalServiceGroupContext groupContext, String prefix) {
		System.out.println(prefix+"GroupContext");
		if (groupContext.getGroupV1().isPresent()) {
			SignalServiceGroup group = groupContext.getGroupV1().get();
			System.out.println(prefix+"  GroupV1");
			if (group.getAvatar().isPresent()) {
				System.out.println(prefix+"    Avatar: TODO");
			}
			byte[] groupId = group.getGroupId();
			System.out.println(prefix+"    GroupId: "+Base64.encodeBytes(groupId));
			if (group.getMembers().isPresent()) {
				System.out.println(prefix+"    Members: TODO");
			}
			if (group.getName().isPresent()) {
				String name = group.getName().get();
				System.out.println(prefix+"    Name: "+name);
			}
			Type type = group.getType();
			System.out.println(prefix+"    Type: "+type.name());
		}
		if (groupContext.getGroupV2().isPresent()) {
			//SignalServiceGroupV2 group = groupContext.getGroupV2().get();
			System.out.println(prefix+"  GroupV2: TODO");
			//group.getMasterKey();
			//group.getRevision();
			//group.getSignedGroupChange();
		}
	}

	private void handleSyncMessage(SignalServiceContent content, String sender) {
		SignalServiceSyncMessage syncMessage = content.getSyncMessage().get();
		if (syncMessage.getSent().isPresent() && syncMessage.getSent().get().getMessage().getBody().isPresent()) {
			SentTranscriptMessage sentTranscriptMessage = syncMessage.getSent().get();
			String chat = sender;
			SignalServiceDataMessage dataMessage = sentTranscriptMessage.getMessage();
			if (dataMessage.getGroupContext().isPresent() && dataMessage.getGroupContext().get().getGroupV1().isPresent()) {
				SignalServiceGroup groupInfo = dataMessage.getGroupContext().get().getGroupV1().get();
				chat = Base64.encodeBytes(groupInfo.getGroupId());
			} else {
				chat = sentTranscriptMessage.getDestination().get().getNumber().get();
			}
		    String message = dataMessage.getBody().get();
			long timestamp = dataMessage.getTimestamp();
		    handleMessageNatively(
	    		this.connection, chat, this.username, message, timestamp, 
	    		PURPLE_MESSAGE_SEND | PURPLE_MESSAGE_REMOTE_SEND | PURPLE_MESSAGE_DELAYED
	    		// flags copied from EionRobb/purple-discord/blob/master/libdiscord.c
	    	);
		} else {
		    handleMessageNatively(
	    		this.connection, sender, sender, "[Received sync message without body.]", 0, PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NO_LOG
	    	);
		}
	}

	private void handleDataMessage(SignalServiceContent content, String source) {
		SignalServiceDataMessage dataMessage = content.getDataMessage().get();
		String chat = source;
		if (dataMessage.getGroupContext().isPresent() && dataMessage.getGroupContext().get().getGroupV1().isPresent()) {
			SignalServiceGroup groupInfo = dataMessage.getGroupContext().get().getGroupV1().get();
			chat = Base64.encodeBytes(groupInfo.getGroupId());
		}
		if (dataMessage.getBody().isPresent()) {
		    String message = dataMessage.getBody().get();
			long timestamp = dataMessage.getTimestamp();
		    handleMessageNatively(this.connection, chat, source, message, timestamp, PURPLE_MESSAGE_RECV);
		    // TODO: do not send receipt until handleMessageNatively returns successfully
		} else {
		    handleMessageNatively(
	    		this.connection, chat, source, "[Received data message without body.]", 0, PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NO_LOG
	    	);
		}
	}
    
    int sendMessage(String who, String message) {
        if (this.manager != null) {
            try {
            	if (who.startsWith("+")) {
            		this.manager.sendMessage(message, null, Arrays.asList(who)); // https://stackoverflow.com/questions/20358883/
            	} else {
                	byte[] groupId = Util.decodeGroupId(who);
                	this.manager.sendGroupMessage(message, null, groupId);
            	}
                return 1;
            } catch (IOException | AttachmentInvalidException | EncapsulatedExceptions | InvalidNumberException | GroupIdFormatException | GroupNotFoundException | NotAGroupMemberException e) {
                e.printStackTrace();
                handleErrorNatively(this.connection, "Exception while sending message: " + e.getMessage());
            }
        }
        return 0;
    }

    static {
        System.loadLibrary("purple-signal"); // will implicitly look for libpurple-signal.so on Linux and purple-signal.dll on Windows
    }

    final int PURPLE_MESSAGE_SEND = 0x0001;
    final int PURPLE_MESSAGE_RECV = 0x0002;
    final int PURPLE_MESSAGE_SYSTEM = 0x0004;
    final int PURPLE_MESSAGE_NICK = 0x0020;
    final int PURPLE_MESSAGE_NO_LOG = 0x0040;
    final int PURPLE_MESSAGE_DELAYED = 0x0400;
    final int PURPLE_MESSAGE_REMOTE_SEND = 0x10000;

    final int DEBUG_LEVEL_INFO = 1; // from libpurple/debug.h
    public static native void logNatively(int level, String text);
    public static native void handleMessageNatively(long connection, String chat, String sender, String content, long timestamp, int flags);
    public static native void handleErrorNatively(long connection, String error);
}
