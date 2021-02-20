package de.hehoe.purple_signal;

import java.security.Security;
import org.bouncycastle.jce.provider.BouncyCastleProvider;
import org.signal.libsignal.metadata.ProtocolInvalidMessageException;
import org.signal.libsignal.metadata.ProtocolNoSessionException;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.List;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.stream.Collectors;

import org.asamk.signal.manager.AttachmentInvalidException;
import org.asamk.signal.manager.Manager;
import org.asamk.signal.manager.Manager.ReceiveMessageHandler;
import org.asamk.signal.manager.NotRegisteredException;
import org.asamk.signal.manager.ProvisioningManager;
import org.asamk.signal.manager.RegistrationManager;
import org.asamk.signal.manager.config.ServiceEnvironment;
import org.asamk.signal.manager.UserAlreadyExists;
import org.asamk.signal.manager.groups.GroupId;
import org.asamk.signal.manager.groups.GroupIdFormatException;
import org.asamk.signal.manager.groups.GroupNotFoundException;
import org.asamk.signal.manager.groups.GroupUtils;
import org.asamk.signal.manager.groups.NotAGroupMemberException;
import org.asamk.signal.manager.storage.SignalAccount;
import org.asamk.signal.manager.storage.contacts.ContactInfo;
import org.asamk.signal.manager.storage.groups.GroupInfo;
import org.asamk.signal.util.SecurityProvider;
import org.asamk.signal.util.Util;
import org.whispersystems.libsignal.InvalidKeyException;
import org.whispersystems.libsignal.InvalidMessageException;
import org.whispersystems.signalservice.api.KeyBackupServicePinException;
import org.whispersystems.signalservice.api.KeyBackupSystemNoDataException;
import org.whispersystems.signalservice.api.SignalServiceMessageReceiver;
import org.whispersystems.signalservice.api.crypto.UntrustedIdentityException;
import org.whispersystems.signalservice.api.messages.SignalServiceAttachment;
import org.whispersystems.signalservice.api.messages.SignalServiceAttachmentPointer;
import org.whispersystems.signalservice.api.messages.SignalServiceContent;
import org.whispersystems.signalservice.api.messages.SignalServiceReceiptMessage;
import org.whispersystems.signalservice.api.messages.SignalServiceDataMessage;
import org.whispersystems.signalservice.api.messages.SignalServiceEnvelope;
import org.whispersystems.signalservice.api.messages.multidevice.SentTranscriptMessage;
import org.whispersystems.signalservice.api.messages.multidevice.SignalServiceSyncMessage;
import org.whispersystems.signalservice.api.push.exceptions.AuthorizationFailedException;
import org.whispersystems.signalservice.api.push.exceptions.MissingConfigurationException;
import org.whispersystems.signalservice.api.util.InvalidNumberException;
import org.whispersystems.signalservice.internal.configuration.SignalServiceConfiguration;

public class PurpleSignal implements ReceiveMessageHandler, Runnable {

	private Manager manager = null;
	private long purpleAccount = 0; // TODO: remove this hack. use number instead. as for thread-safety, the callbacks do a lookup anyway.
	private boolean keepReceiving = false;
	private Thread receiverThread = null;
	private String username = null;
	private final ServiceEnvironment serviceEnvironment;
	private final File dataPath = null;
	// dataPath is not actually being used. All data is redirected to libpurple's internal storage
	// (see adjustments made to org.asamk.signal.manager.storageSignalAccount in signal-cli submodule).
	// dataPath remains in some method calls so the signatures remain the same.
	private RegistrationManager registrationManager;

	private class BaseConfig {
		private static final String USER_AGENT = "purple-signal";
	}

	public PurpleSignal(long purpleAccount, String username)
			throws IOException, TimeoutException, InvalidKeyException, UserAlreadyExists, NotRegisteredException {
		serviceEnvironment = ServiceEnvironment.LIVE;

		this.purpleAccount = purpleAccount;
		this.username = username;
		this.keepReceiving = false;

		// stolen from signald/src/main/java/io/finn/signald/Main.java
		// Workaround for BKS truststore
		Security.insertProviderAt(new SecurityProvider(), 1);
		Security.addProvider(new BouncyCastleProvider());

		if (!SignalAccount.userExists(this.purpleAccount)) {
			askRegisterOrLinkNatively(this.purpleAccount);
		} else {
			// the rest is adapted from signal-cli/src/main/java/org/asamk/signal/Main.java

			this.manager = Manager.init(username, dataPath, serviceEnvironment, BaseConfig.USER_AGENT);
			// exceptions from init not caught, may bubble to C++

			{
				try {
					this.manager.checkAccountState();
				} catch (AuthorizationFailedException e) {
					logNatively(DEBUG_LEVEL_INFO, "Authorization failed, was the number registered elsewhere?");
					askRegisterOrLinkNatively(this.purpleAccount);
				}
				// other exceptions may bubble to C++
			}

			final String profileName = getSettingsStringNatively(this.purpleAccount, "profile-name", "");
			if (!profileName.isEmpty()) {
				this.manager.setProfile(profileName, null, null, null);
				// exception may bubble to C++
			}
			
			logNatively(DEBUG_LEVEL_INFO, "User " + manager.getUsername() + " is authorized.");
			// TODO: use signal-cli's new getUserStatus to get actual status
			// TODO: signal account "connected"
			startReceiving();
			
			updateLocalGroups();
		}
	}

	public void updateLocalGroups() {
		// this does not actually do anything
		// right after login, the group store is always empty
		// TODO: find out why this is the case. find out how to request groups from the server / master device
        for (GroupInfo group : this.manager.getGroups()) {
        	GroupId groupId = group.getGroupId();
        	String chat = groupId.toBase64();
        	String title = group.getTitle();
        	System.err.println("Got group "+groupId+" with title "+title+".");
        	handleContactNatively(this.purpleAccount, chat, title);
        	if (PurpleSignalGroupUtils.resolveMembers(this.manager, group.getPendingMembers()).contains(this.username)) {
    			joinGroup(groupId, chat, this.username);
        	}
        }
	}
	
	public void linkAccount() throws TimeoutException, IOException {
		ProvisioningManager provisioningManager = ProvisioningManager.init(dataPath, serviceEnvironment,
				BaseConfig.USER_AGENT);
		String deviceLinkUri = provisioningManager.getDeviceLinkUri();
		handleQRCodeNatively(this.purpleAccount, deviceLinkUri);
		receiverThread = new Thread(() -> {
			String linkedUsername = null;
			try {
				linkedUsername = provisioningManager.finishDeviceLink("purple-signal");
				if (!this.username.equals(linkedUsername)) {
					handleErrorNatively(this.purpleAccount,
						String.format("Configured username %s does not match linked number %s.",
							this.username,
							linkedUsername));
				} else {
					handleErrorNatively(this.purpleAccount, "Linking finished. Reconnect needed.");
				}
			} catch (UserAlreadyExists e) {
				handleErrorNatively(this.purpleAccount,
					"Unable to finish device link: User already exists locally. Delete Pidgin/libpurple account and try again.");
			} catch (Throwable t) {
				handleErrorNatively(this.purpleAccount,
					"Unable to finish device link due to " + t.getClass().getName() + ": " + t.getMessage());
				t.printStackTrace();
			}

		});
		receiverThread.setName("finishDeviceLink");
		receiverThread.setDaemon(true);
		receiverThread.start();
	}

	public void registerAccount(boolean voiceVerification, String captcha) throws IOException {
		if (this.registrationManager == null) {
			this.registrationManager = RegistrationManager.init(username, dataPath, serviceEnvironment, BaseConfig.USER_AGENT);
		}
		this.registrationManager.register(voiceVerification, captcha);
		askVerificationCodeNatively(this.purpleAccount);
	}

	public void verifyAccount(String verificationCode, String captcha)
			throws IOException, KeyBackupServicePinException, KeyBackupSystemNoDataException {
		if (captcha.isEmpty()) {
			captcha = null;
		}
		this.registrationManager.verifyAccount(verificationCode, captcha);
		handleErrorNatively(this.purpleAccount, "Verification finished. Reconnect needed.");
	}

	public void run() {
		logNatively(DEBUG_LEVEL_INFO, "Starting to listen for messages…");
		boolean ignoreAttachments = true;
		boolean returnOnTimeout = true; // it looks like setting this to false means "listen for new messages forever".
		// There seems to be a non-daemon thread to be involved somewhere as the Java VM will not ever shut down.
		long timeout = 60; // Seconds to wait for an incoming message. After the timeout occurred, a re-connect happens silently.
		// TODO: Find out how this affects what.
		try {
			while (this.keepReceiving) {
				this.manager.receiveMessages((long) (timeout * 1000),
					TimeUnit.MILLISECONDS,
					returnOnTimeout,
					ignoreAttachments,
					this);
			}
		} catch (NoClassDefFoundError e) {
			if (e.getMessage().contains("org.signal.zkgroup.internal.Native")) {
				handleErrorNatively(this.purpleAccount,
					"Unable to load zkgroup library required for group conversation.",
					false);
			} else {
				handleErrorNatively(this.purpleAccount, e.getMessage(), false);
			}
		} catch (Exception e) {
			handleErrorNatively(this.purpleAccount, e.getClass().getName() + " while waiting for message: " + e.getMessage(), false);
			e.printStackTrace();
		} catch (Throwable t) {
			handleErrorNatively(this.purpleAccount, "Unhandled " + t.getClass().getName() + " while waiting for message.");
			t.printStackTrace();
		}
		logNatively(DEBUG_LEVEL_INFO, "Receiving has finished.");
		// TODO: signal "disconnected" to UI
	}

	private void startReceiving() {
		if (receiverThread != null) {
			handleErrorNatively(this.purpleAccount,
				"Called startReceiving() on a connection already receiving. This is a bug.");
		} else {
			this.keepReceiving = true;
			receiverThread = new Thread(this);
			receiverThread.setName("Receiver");
			receiverThread.setDaemon(true);
			receiverThread.start();
		}
	}

	public void stopReceiving() {
		this.keepReceiving = false;
		if (this.manager != null) {
			try {
				this.manager.close();
				this.manager = null;
			} catch (IOException e) {
				// I don't care about what dying managers have to say
			}
		}
		if (receiverThread != null) {
			try {
				receiverThread.join();
			} catch (InterruptedException e) {
				// I don't care about what dying connections have to say
			}
		}
	}

	@Override
	public void handleMessage(SignalServiceEnvelope envelope, SignalServiceContent content, Throwable exception) {
		logNatively(DEBUG_LEVEL_INFO, "RECEIVED SOMETHING!");
		// stolen from signald/src/main/java/io/finn/signald/MessageReceiver.java and
		// signal-cli/src/main/java/org/asamk/signal/JsonMessageEnvelope.java and
		// signal-cli/src/main/java/org/asamk/signal/ReceiveMessageHandler.java
		Throwable cause = null;
		if (exception != null) {
			if (exception instanceof ProtocolNoSessionException) {
				handleErrorNatively(this.purpleAccount,
					"No Session. If problem persists, delete local account. Link or register again.");
			} else if (exception instanceof ProtocolInvalidMessageException
					&& (cause = ((ProtocolInvalidMessageException) exception)
							.getCause()) instanceof InvalidMessageException
					&& ((org.whispersystems.libsignal.InvalidMessageException) cause).getMessage()
							.equals("No valid sessions.")) {
				handleErrorNatively(this.purpleAccount,
					"No valid sessions. Delete local account. Link or register again.");
			} else {
				handleErrorNatively(this.purpleAccount,
					exception.getClass().getName() + " while handling message: " + exception.getMessage(),
					false);
			}
		}
		if (envelope == null) {
			handleErrorNatively(this.purpleAccount, "Handling null envelope."); // this should never happen
		} else {
			SignalMessagePrinter.printEnvelope(envelope);
			long timestamp = envelope.getTimestamp();
			String source = null;
			if (envelope.isUnidentifiedSender()) {
				logNatively(DEBUG_LEVEL_INFO, "Envelope shows unidentified sender.");
				if (content == null) {
					logNatively(DEBUG_LEVEL_INFO, "Message has no readable content.");
				} else {
					logNatively(DEBUG_LEVEL_INFO, "Using sender from content.");
					source = content.getSender().getNumber().orNull();
				}
			} else {
				source = envelope.getSourceAddress().getNumber().orNull();
			}
			if (source == null) {
				logNatively(DEBUG_LEVEL_INFO, "Source is null. Ignoring message.");
			} else if (envelope.isReceipt()) {
				logNatively(DEBUG_LEVEL_INFO, "Ignoring receipt.");
			} else if (content == null) {
				handleMessageNatively(this.purpleAccount,
					source,
					source,
					"Failed to decrypt incoming message.",
					timestamp,
					PURPLE_MESSAGE_ERROR);
			} else {
				SignalMessagePrinter.printSignalServiceContent(content);
				if (content.getDataMessage().isPresent()) {
					handleDataMessage(content, source);
					// TODO: do not send receipt until handleMessageNatively returns successfully
					try {
						this.manager.sendReceipt(content.getSender(),
							content.getDataMessage().get().getTimestamp(),
							SignalServiceReceiptMessage.Type.READ);
					} catch (IOException | UntrustedIdentityException e) {
						logNatively(DEBUG_LEVEL_INFO, "Receipt was not sent successfully: " + e);
					}
				} else if (content.getSyncMessage().isPresent()) {
					handleSyncMessage(content, source);
				} else if (content.getTypingMessage().isPresent()) {
					logNatively(DEBUG_LEVEL_INFO, "Received typing message for " + source + ". Ignoring.");
				} else if (content.getReceiptMessage().isPresent()) {
					String description = "[Message "
							+ content.getReceiptMessage().get().getType().toString().toLowerCase() + ".]";
					handleMessageNatively(this.purpleAccount,
						source,
						source,
						description,
						timestamp,
						PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NO_LOG);
				} else {
					handleMessageNatively(this.purpleAccount,
						source,
						source,
						"[Received message of unknown type.]",
						timestamp,
						PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NO_LOG);
				}
				// TODO: support all message types
				// TODO: find out whether message types are mutually exclusive (they seem to be – but is it guaranteed?)
			}
		}
	}

	private void handleSyncMessage(SignalServiceContent content, String sender) {
		SignalServiceSyncMessage syncMessage = content.getSyncMessage().get();
		if (syncMessage.getSent().isPresent() && syncMessage.getSent().get().getMessage().getBody().isPresent()) {
			// TODO: handle attachments
			SentTranscriptMessage sentTranscriptMessage = syncMessage.getSent().get();
			String chat = sender;
			SignalServiceDataMessage dataMessage = sentTranscriptMessage.getMessage();
			if (dataMessage.getGroupContext().isPresent()) {
				chat = GroupUtils.getGroupId(dataMessage.getGroupContext().get()).toBase64();
			} else {
				chat = sentTranscriptMessage.getDestination().get().getNumber().get();
			}
			String message = dataMessage.getBody().get();
			long timestamp = dataMessage.getTimestamp();
			handleMessageNatively(this.purpleAccount,
				chat,
				this.username,
				message,
				timestamp,
				PURPLE_MESSAGE_SEND | PURPLE_MESSAGE_REMOTE_SEND | PURPLE_MESSAGE_DELAYED
			// flags copied from EionRobb/purple-discord/blob/master/libdiscord.c
			);
		} else if (syncMessage.getContacts().isPresent()) {
			List<ContactInfo> contacts = this.manager.account.getContactStore().getContacts();
			System.err.println("Current contacts:");
			for (ContactInfo contact : contacts) {
				System.err.println("Number:" + contact.number + ", Name:" + contact.name);
				handleContactNatively(this.purpleAccount, contact.number, contact.name);
			}
		} else {
			handleMessageNatively(this.purpleAccount,
				sender,
				sender,
				"[Received sync message without body.]",
				0,
				PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NO_LOG);
		}
	}

	private void handleDataMessage(SignalServiceContent content, String source) {
		SignalServiceDataMessage dataMessage = content.getDataMessage().get();
		String chat = source;
		if (dataMessage.getGroupContext().isPresent()) {
			chat = GroupUtils.getGroupId(dataMessage.getGroupContext().get()).toBase64();
		}
		long timestamp = dataMessage.getTimestamp();
		if (dataMessage.getBody().isPresent()) {
			String message = dataMessage.getBody().get();
			handleMessageNatively(this.purpleAccount, chat, source, message, timestamp, PURPLE_MESSAGE_RECV);
		}
		if (dataMessage.getAttachments().isPresent()) {
			for (SignalServiceAttachment attachment : dataMessage.getAttachments().get()) {
				SignalServiceAttachmentPointer attachmentPtr = attachment.asPointer();
				if (attachmentPtr.getPreview().isPresent()) {
					handlePreviewNatively(this.purpleAccount,
						chat,
						source,
						attachmentPtr.getPreview().get(),
						timestamp,
						PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NO_LOG);
				}
				if (attachmentPtr.getSize().isPresent()) {
					final int fileSize = attachmentPtr.getSize().get();
					final String fileName = attachmentPtr.getFileName().get();
					handleAttachmentNatively(this.purpleAccount, chat, source, attachmentPtr, fileName, fileSize);
				} else {
					handleMessageNatively(this.purpleAccount,
						chat,
						source,
						"[Received data message with attachment without size.]",
						timestamp,
						PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NO_LOG);
				}
			}
		}
		if (!dataMessage.getBody().isPresent() && !dataMessage.getAttachments().isPresent()) {
			handleBodilessMessage(chat, source, dataMessage);
		}
	}
	
	private void joinGroup(GroupId groupId, String chat, String source) {
		try {
			// TODO: ask user before joining group
			this.manager.updateGroup(groupId, null, null, null);
			handleMessageNatively(this.purpleAccount,
				chat,
				source,
				"[I updated (probably joined) this group.]",
				0,
				PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NO_LOG);
		} catch (IOException | GroupNotFoundException | AttachmentInvalidException | NotAGroupMemberException
				| InvalidNumberException e) {
			handleErrorNatively(this.purpleAccount,
				e.getClass().getName() + " while updating group: " + e.getMessage(),
				false);
			e.printStackTrace();
		}
	}

	private void handleBodilessMessage(String chat, String source, SignalServiceDataMessage dataMessage) {
		if (dataMessage.getGroupContext().isPresent()) {
			GroupId groupId = GroupUtils.getGroupId(dataMessage.getGroupContext().get());
			joinGroup(groupId, chat, source);
		} else {
			handleMessageNatively(this.purpleAccount,
				chat,
				source,
				"[Received data message with neither body nor attachment.]",
				0,
				PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NO_LOG);
		}
	}

	public Object acceptAttachment(Object attachment, String localFileName)
			throws IOException, InvalidMessageException, MissingConfigurationException {
		if (attachment != null) {
			SignalServiceAttachmentPointer attachmentPtr = (SignalServiceAttachmentPointer) attachment;
			File tmpFile = new File(localFileName + ".tmp"); // or File tmpFile = IOUtils.createTempFile();
			final SignalServiceMessageReceiver messageReceiver = this.manager.messageReceiver;
			final InputStream attachmentStream = messageReceiver
					.retrieveAttachment(attachmentPtr, tmpFile, 150 * 1024 * 1024); // from ServiceConfig.MAX_ATTACHMENT_SIZE
			tmpFile.delete(); // this should have effect on disk only after all handles have been closed
			return attachmentStream;
		} else {
			return null;
		}
	}

	public int sendMessage(String who, String message) {
		if (this.manager != null) {
			try {
				if (who.startsWith("+")) {
					this.manager.sendMessage(message, null, Arrays.asList(who)); // https://stackoverflow.com/questions/20358883/
				} else {
					GroupId groupId = Util.decodeGroupId(who);
					this.manager.sendGroupMessage(message, null, groupId);
				}
				return 1;
			} catch (IOException | AttachmentInvalidException | InvalidNumberException | GroupIdFormatException
					| GroupNotFoundException | NotAGroupMemberException e) {
				e.printStackTrace();
				handleErrorNatively(this.purpleAccount, "Exception while sending message: " + e.getMessage());
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
	final int PURPLE_MESSAGE_ERROR = 0x0200;
	final int PURPLE_MESSAGE_DELAYED = 0x0400;
	final int PURPLE_MESSAGE_REMOTE_SEND = 0x10000;

	// from libpurple/debug.h:
	public final static int DEBUG_LEVEL_INFO = 2;
	public final static int DEBUG_LEVEL_ERROR = 4;

	public static native void logNatively(int level, String text);

	public static native void handleMessageNatively(long account, String chat, String sender, String content,
			long timestamp, int flags);

	public static native void handlePreviewNatively(long account, String chat, String sender, byte[] preview,
			long timestamp, int flags);

	public static native void handleAttachmentNatively(long account, String chat, String sender,
			SignalServiceAttachmentPointer attachmentPtr, String fileName, int fileSize);

	public static native void handleContactNatively(long account, String number, String name);
	
	public static native void handleErrorNatively(long account, String error, boolean fatal);

	public static void handleErrorNatively(long account, String error) {
		handleErrorNatively(account, error, true);
	}

	public static native void askRegisterOrLinkNatively(long account);

	public static native void handleQRCodeNatively(long account, String deviceLinkUri);

	public static native void askVerificationCodeNatively(long account);

	public static native String getSettingsStringNatively(long account, String key, String defaultValue);

	public static native void setSettingsStringNatively(long account, String key, String value);

	public static long lookupAccountByUsername(String username) throws IOException {
		long account = lookupAccountByUsernameNatively(username);
		if (account == 0) {
			throw new IOException("No account exists for this username.");
		}
		return account;
	}

	public static native long lookupAccountByUsernameNatively(String username);
}
