package de.hehoe.purple_signal;

/*
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.core.config.Configurator;
*/

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
import org.whispersystems.signalservice.api.push.SignalServiceAddress;

import org.asamk.signal.util.IOUtils;

public class Main {

	//private static final Logger logger = LogManager.getLogger("signald");

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
		// stolen from signald/src/main/java/io/finn/signald/Main.java and signal-cli/src/main/java/org/asamk/signal/Main.java

		//Logger logger = LogManager.getLogger("purple-signal");
		//Configurator.setLevel(System.getProperty("log4j.logger"), Level.DEBUG);

		// Workaround for BKS truststore
		Security.insertProviderAt(new SecurityProvider(), 1);
		Security.addProvider(new BouncyCastleProvider());

		String data_path = getDefaultDataPath();
		//logger.debug("Using data folder " + data_path);
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
							// stolen from signald/src/main/java/io/finn/signald/MessageReceiver.java
					        if (exception != null) {
					            // TODO: forward exception
					        }
							if (envelope != null) {
								SignalServiceAddress source = envelope.getSourceAddress();
								System.out.println(source.getNumber());
								System.out.println(envelope);
								System.out.println(content);
								System.out.println(exception);
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
