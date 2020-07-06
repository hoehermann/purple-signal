package de.hehoe.purple_signal;

import java.security.Security;
import org.bouncycastle.jce.provider.BouncyCastleProvider;

import java.io.File;
import java.io.IOException;
import java.util.Arrays;
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
import org.whispersystems.signalservice.api.messages.SignalServiceContent;
import org.whispersystems.signalservice.api.messages.SignalServiceDataMessage;
import org.whispersystems.signalservice.api.messages.SignalServiceEnvelope;
import org.whispersystems.signalservice.api.messages.SignalServiceGroup;
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
            handleMessageNatively(this.connection, "link", "Please use this code to link this Pidgin account (use a QR encoder for linking with real phones):<br/>"+deviceLinkUri, 0);
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
	                this.manager.receiveMessages((long) (timeout * 1000), TimeUnit.MILLISECONDS, returnOnTimeout,
	                    ignoreAttachments, this);
	            }
	        } catch (Exception e) {
	            handleErrorNatively(this.connection, "Exception while waiting for or receiving message: " + e.getMessage());
	        } catch (Throwable t) {
	            handleErrorNatively(this.connection, "Exception while waiting for or receiving message.");
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
            SignalServiceAddress source = envelope.getSourceAddress();
            String who = source.getNumber().orNull();
            if (who == null) {
            	logNatively(DEBUG_LEVEL_INFO, "Source is null – ignoring message.");
            } else if (content == null) {
            	logNatively(DEBUG_LEVEL_INFO, "Failed to decrypt incoming message – ignoring.");
                //handleErrorNatively(this.connection, "Failed to decrypt incoming message.");
            } else {
                boolean isReceipt = envelope.isReceipt();
                if (isReceipt) {
                    // TODO: display receipts as system-messages
                } else {
                    if (content.getDataMessage().isPresent()) {
                        handleDataMessage(content, who);
                    }
                }
            }
            // TODO: support other message types
        }

    }

	private void handleDataMessage(SignalServiceContent content, String who) {
		SignalServiceDataMessage dataMessage = content.getDataMessage().get();
		if (dataMessage.getGroupContext().isPresent() && dataMessage.getGroupContext().get().getGroupV1().isPresent()) {
			SignalServiceGroup groupInfo = dataMessage.getGroupContext().get().getGroupV1().get();
			who = Base64.encodeBytes(groupInfo.getGroupId());
		}
		long timestamp = dataMessage.getTimestamp();
		if (dataMessage.getBody().isPresent()) {
		    String message = dataMessage.getBody().get();
		    handleMessageNatively(this.connection, who, message, timestamp);
		    // TODO: do not send receipt until handleMessageNatively returns successfully
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

    final int DEBUG_LEVEL_INFO = 1; // from libpurple/debug.h
    public static native void logNatively(int level, String text);
    public static native void handleMessageNatively(long connection, String who, String content, long timestamp);
    public static native void handleErrorNatively(long connection, String error);
}
