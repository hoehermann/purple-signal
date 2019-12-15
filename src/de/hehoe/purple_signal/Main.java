package de.hehoe.purple_signal;

import java.security.Security;
import org.bouncycastle.jce.provider.BouncyCastleProvider;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.TimeUnit;

import org.asamk.signal.AttachmentInvalidException;
import org.asamk.signal.GroupNotFoundException;
import org.asamk.signal.NotAGroupMemberException;
import org.asamk.signal.manager.Manager;
import org.asamk.signal.util.SecurityProvider;
import org.whispersystems.signalservice.api.messages.SignalServiceDataMessage;
import org.whispersystems.signalservice.api.messages.SignalServiceGroup;
import org.whispersystems.signalservice.api.push.SignalServiceAddress;
import org.whispersystems.signalservice.internal.util.Base64;
import org.asamk.signal.util.IOUtils;

public class Main {


	// stolen from signal-cli/src/main/java/org/asamk/signal/Main.java
	/**
	 * Uses $XDG_DATA_HOME/signal-cli if it exists, or if none of the legacy
	 * directories exist: - $HOME/.config/signal - $HOME/.config/textsecure
	 *
	 * @return the data directory to be used by signal-cli.
	 */
	 private static String getDefaultDataPath() { String dataPath =
	 IOUtils.getDataHomeDir() + "/signal-cli"; if (new File(dataPath).exists()) {
	 return dataPath; }
	 
	 String legacySettingsPath = System.getProperty("user.home") +
	 "/.config/signal"; if (new File(legacySettingsPath).exists()) { return
	 legacySettingsPath; }
	 
	 legacySettingsPath = System.getProperty("user.home") + "/.config/textsecure";
	 if (new File(legacySettingsPath).exists()) { return legacySettingsPath; }
	 
	 return dataPath; }
	 

	public static void main(String[] args) {
		// stolen from signald/src/main/java/io/finn/signald/Main.java

		// Workaround for BKS truststore
		Security.insertProviderAt(new SecurityProvider(), 1);
		Security.addProvider(new BouncyCastleProvider());

		String data_path = getDefaultDataPath();
		System.out.println("Using data folder " + data_path);
		Manager m = new Manager("+49[REDACTED]", data_path);
		try {
			m.init();

			System.out.println(m.getUsername());
			long timeout = 5;
			boolean returnOnTimeout = true;
			boolean ignoreAttachments = true;
			try {
				m.receiveMessages((long) (timeout * 1000), TimeUnit.MILLISECONDS, returnOnTimeout, ignoreAttachments,
						(envelope, content, exception) -> {
							System.out.println("RECEIVED!");
							// stolen from signald/src/main/java/io/finn/signald/MessageReceiver.java and signal-cli/src/main/java/org/asamk/signal/JsonMessageEnvelope.java
					        if (exception != null) {
					            // TODO: forward exception
					        }
							if (envelope != null && content != null) {
								SignalServiceAddress source = envelope.getSourceAddress();
						        String who = source.getNumber();
						        long timestamp = envelope.getTimestamp();
						        boolean isReceipt = envelope.isReceipt();
						        if (!isReceipt) {
						            if (content.getDataMessage().isPresent()) {
						                SignalServiceDataMessage dataMessage = content.getDataMessage().get();
						                timestamp = dataMessage.getTimestamp();
						                if (dataMessage.getGroupInfo().isPresent()) {
						                    SignalServiceGroup groupInfo = dataMessage.getGroupInfo().get();
						                    who = Base64.encodeBytes(groupInfo.getGroupId()); // TODO: support groups properly
						                }
						                if (dataMessage.getBody().isPresent()) {
						                    String message = dataMessage.getBody().get();
											System.out.println(who);
						                    System.out.println(timestamp);
											System.out.println(message);
						                }
						            }
					            }
					            // TODO: support other message types
							}
						});
			} catch (IOException | NotAGroupMemberException | GroupNotFoundException | AttachmentInvalidException e) {
				// TODO forward exception
			}
			System.out.println("DONE");
		} catch (Exception e) {
			System.err.println("Error loading state file: " + e.getMessage());
		}
	}

}
