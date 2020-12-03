package de.hehoe.purple_signal;

import java.security.Security;
import org.bouncycastle.jce.provider.BouncyCastleProvider;

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
import org.whispersystems.signalservice.api.crypto.UntrustedIdentityException;
import org.whispersystems.signalservice.api.messages.SignalServiceContent;
import org.whispersystems.signalservice.api.messages.SignalServiceReceiptMessage;
import org.whispersystems.signalservice.api.messages.SignalServiceDataMessage;
import org.whispersystems.signalservice.api.messages.SignalServiceEnvelope;
import org.whispersystems.signalservice.api.messages.SignalServiceGroup;
import org.whispersystems.signalservice.api.messages.multidevice.SentTranscriptMessage;
import org.whispersystems.signalservice.api.messages.multidevice.SignalServiceSyncMessage;
import org.whispersystems.signalservice.api.push.exceptions.AuthorizationFailedException;
import org.whispersystems.signalservice.api.push.exceptions.EncapsulatedExceptions;
import org.whispersystems.signalservice.api.util.InvalidNumberException;
import org.whispersystems.signalservice.internal.configuration.SignalServiceConfiguration;
import org.whispersystems.util.Base64;
import org.asamk.signal.util.GroupIdFormatException;

public class PurpleSignal implements ReceiveMessageHandler, Runnable {

	private Manager manager = null;
	private long connection = 0;
	private boolean keepReceiving = false;
	private Thread receiverThread = null;
	private String username = null;
	private final SignalServiceConfiguration serviceConfiguration;
	private final String dataPath;

	private class BaseConfig {
		private static final String USER_AGENT = "purple-signal";
	}

	public PurpleSignal(long connection, long account, String username, String dataPath)
			throws IOException, TimeoutException, InvalidKeyException, UserAlreadyExists {
		serviceConfiguration = ServiceConfig.createDefaultServiceConfiguration(BaseConfig.USER_AGENT);

		this.connection = connection;
		this.username = username;
		this.keepReceiving = false;
		this.dataPath = dataPath;

		// stolen from signald/src/main/java/io/finn/signald/Main.java
		// Workaround for BKS truststore
		Security.insertProviderAt(new SecurityProvider(), 1);
		Security.addProvider(new BouncyCastleProvider());

		if (!SignalAccount.userExists(account)) {
			askRegisterOrLinkNatively(this.connection);
		} else {

			// the rest is adapted from signal-cli/src/main/java/org/asamk/signal/Main.java

			this.manager = Manager.init(username, dataPath, serviceConfiguration, BaseConfig.USER_AGENT);
			// exception may bubble to C++

			{
				Manager m = this.manager;
				try {
					m.checkAccountState();
				} catch (AuthorizationFailedException e) {
					logNatively(DEBUG_LEVEL_INFO, "Authorization failed, was the number registered elsewhere?");
					askRegisterOrLinkNatively(this.connection);
				}
				// other exceptions may bubble to C++
			}

			logNatively(DEBUG_LEVEL_INFO, "User " + manager.getUsername() + " is authorized.");
			startReceiving();

			// TODO: signal account "connected"
		}
	}

	public void linkAccount() throws TimeoutException, IOException {
		ProvisioningManager provisioningManager = new ProvisioningManager(dataPath, serviceConfiguration,
				BaseConfig.USER_AGENT);
		String deviceLinkUri = provisioningManager.getDeviceLinkUri();
		handleQRCodeNatively(this.connection, deviceLinkUri);
		receiverThread = new Thread(() -> {
			String linkedUsername = null;
			try {
				linkedUsername = provisioningManager.finishDeviceLink("purple-signal");
			} catch (IOException | InvalidKeyException | TimeoutException | UserAlreadyExists e) {
				handleErrorNatively(this.connection, "Unable to finish device link: " + e.getMessage());
				e.printStackTrace();
			}
			if (!linkedUsername.equals(this.username)) {
				handleErrorNatively(this.connection, String.format(
						"Configured username %s does not match linked number %s.", this.username, linkedUsername));
			} else {
				handleErrorNatively(this.connection, "Linking finished. Reconnect needed.");
			}

		});
		receiverThread.setName("finishDeviceLink");
		receiverThread.setDaemon(true);
		receiverThread.start();
	}

	public void registerAccount(boolean voiceVerification, String captcha) throws IOException {
		if (this.manager == null) {
			this.manager = Manager.init(username, dataPath, serviceConfiguration, BaseConfig.USER_AGENT);
		}
		this.manager.register(voiceVerification, captcha);
		askVerificationCodeNatively(this.connection);
	}

	public void verifyAccount(String verificationCode, String pin) throws IOException {
		this.manager.verifyAccount(verificationCode, pin);
		handleErrorNatively(this.connection, "Verification finished. Reconnect needed.");
	}

	public void run() {
		logNatively(DEBUG_LEVEL_INFO, "Starting to listen for messages…");
		boolean ignoreAttachments = true;
		boolean returnOnTimeout = true; // it looks like setting this to false means "listen for new messages
										// forever".
		// There seems to be a non-daemon thread to be involved somewhere as the Java VM
		// will not ever shut down.
		long timeout = 60; // Seconds to wait for an incoming message. After the timeout occurred, a
							// re-connect happens silently.
		// TODO: Find out how this affects what.
		try {
			while (this.keepReceiving) {
				this.manager.receiveMessages((long) (timeout * 1000), TimeUnit.MILLISECONDS, returnOnTimeout,
						ignoreAttachments, this);
			}
		} catch (Exception e) {
			handleErrorNatively(this.connection, "Exception while waiting for message: " + e.getMessage());
		} catch (Throwable t) {
			handleErrorNatively(this.connection, "Unhandled exception while waiting for message.");
			t.printStackTrace();
		}
		logNatively(DEBUG_LEVEL_INFO, "Receiving has finished.");
	}

	private void startReceiving() {
		if (receiverThread != null) {
			handleErrorNatively(this.connection,
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
		if (exception != null) {
			handleErrorNatively(this.connection, "Exception while handling message: " + exception.getMessage());
		} else if (envelope == null) {
			handleErrorNatively(this.connection, "Handling null envelope."); // this should never happen
		} else {
			SignalMessagePrinter.printEnvelope(envelope);
			String source = null;
			if (envelope.isUnidentifiedSender()) {
				logNatively(DEBUG_LEVEL_INFO, "Envelope shows unidentified sender. Using sender from content.");
				source = content.getSender().getNumber().orNull();
			} else {
				source = envelope.getSourceAddress().getNumber().orNull();
			}
			if (source == null) {
				logNatively(DEBUG_LEVEL_INFO, "Source is null. Ignoring message.");
			} else if (envelope.isReceipt()) {
				logNatively(DEBUG_LEVEL_INFO, "Ignoring receipt.");
			} else if (content == null) {
				logNatively(DEBUG_LEVEL_INFO, "Failed to decrypt incoming message. Ignoring message.");
				// handleErrorNatively(this.connection, "Failed to decrypt incoming message.");
			} else {
				long timestamp = envelope.getTimestamp();
				SignalMessagePrinter.printSignalServiceContent(content);
				if (content.getDataMessage().isPresent()) {
					handleDataMessage(content, source);
					// TODO: do not send receipt until handleMessageNatively returns successfully
					try {
						this.manager.sendReceipt(content.getSender(), content.getDataMessage().get().getTimestamp(), SignalServiceReceiptMessage.Type.READ);
					} catch (IOException | UntrustedIdentityException e) {
						logNatively(DEBUG_LEVEL_INFO, "Receipt was not sent successfully: " + e);
					}
				} else if (content.getSyncMessage().isPresent()) {
					handleSyncMessage(content, source);
				} else if (content.getTypingMessage().isPresent()) {
					logNatively(DEBUG_LEVEL_INFO, "Received typing message for " + source + ". Ignoring.");
				} else if (content.getReceiptMessage().isPresent()) {
					String description = "[Message "+content.getReceiptMessage().get().getType().toString().toLowerCase()+".]";
					handleMessageNatively(this.connection, source, source, description, timestamp,
							PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NO_LOG);
				} else {
					handleMessageNatively(this.connection, source, source, "[Received message of unknown type.]",
							timestamp, PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NO_LOG);
				}
				// TODO: support all message types
			}
		}
	}

	private void handleSyncMessage(SignalServiceContent content, String sender) {
		SignalServiceSyncMessage syncMessage = content.getSyncMessage().get();
		if (syncMessage.getSent().isPresent() && syncMessage.getSent().get().getMessage().getBody().isPresent()) {
			SentTranscriptMessage sentTranscriptMessage = syncMessage.getSent().get();
			String chat = sender;
			SignalServiceDataMessage dataMessage = sentTranscriptMessage.getMessage();
			if (dataMessage.getGroupContext().isPresent()
					&& dataMessage.getGroupContext().get().getGroupV1().isPresent()) {
				SignalServiceGroup groupInfo = dataMessage.getGroupContext().get().getGroupV1().get();
				chat = Base64.encodeBytes(groupInfo.getGroupId());
			} else {
				chat = sentTranscriptMessage.getDestination().get().getNumber().get();
			}
			String message = dataMessage.getBody().get();
			long timestamp = dataMessage.getTimestamp();
			handleMessageNatively(this.connection, chat, this.username, message, timestamp,
					PURPLE_MESSAGE_SEND | PURPLE_MESSAGE_REMOTE_SEND | PURPLE_MESSAGE_DELAYED
			// flags copied from EionRobb/purple-discord/blob/master/libdiscord.c
			);
		} else {
			handleMessageNatively(this.connection, sender, sender, "[Received sync message without body.]", 0,
					PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NO_LOG);
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
		} else {
			handleMessageNatively(this.connection, chat, source, "[Received data message without body.]", 0,
					PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NO_LOG);
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
			} catch (IOException | AttachmentInvalidException | InvalidNumberException
					| GroupIdFormatException | GroupNotFoundException | NotAGroupMemberException e) {
				e.printStackTrace();
				handleErrorNatively(this.connection, "Exception while sending message: " + e.getMessage());
			}
		}
		return 0;
	}

	static {
		System.loadLibrary("purple-signal"); // will implicitly look for libpurple-signal.so on Linux and
												// purple-signal.dll on Windows
	}

	final int PURPLE_MESSAGE_SEND = 0x0001;
	final int PURPLE_MESSAGE_RECV = 0x0002;
	final int PURPLE_MESSAGE_SYSTEM = 0x0004;
	final int PURPLE_MESSAGE_NICK = 0x0020;
	final int PURPLE_MESSAGE_NO_LOG = 0x0040;
	final int PURPLE_MESSAGE_DELAYED = 0x0400;
	final int PURPLE_MESSAGE_REMOTE_SEND = 0x10000;

	final static int DEBUG_LEVEL_INFO = 1; // from libpurple/debug.h

	public static native void logNatively(int level, String text);

	public static native void handleMessageNatively(long connection, String chat, String sender, String content,
			long timestamp, int flags);

	public static native void handleErrorNatively(long connection, String error);

	public static native void askRegisterOrLinkNatively(long connection);

	public static native void handleQRCodeNatively(long connection, String deviceLinkUri);

	public static native void askVerificationCodeNatively(long connection);
	
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
