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
import org.asamk.signal.manager.Manager.ReceiveMessageHandler;
import org.asamk.signal.util.SecurityProvider;
import org.whispersystems.signalservice.api.messages.SignalServiceContent;
import org.whispersystems.signalservice.api.messages.SignalServiceDataMessage;
import org.whispersystems.signalservice.api.messages.SignalServiceEnvelope;
import org.whispersystems.signalservice.api.messages.SignalServiceGroup;
import org.whispersystems.signalservice.api.push.SignalServiceAddress;
import org.whispersystems.signalservice.internal.util.Base64;
import org.asamk.signal.util.IOUtils;

public class PurpleSignal implements ReceiveMessageHandler, Runnable {

    // stolen from signal-cli/src/main/java/org/asamk/signal/Main.java
    /**
     * Uses $XDG_DATA_HOME/signal-cli if it exists, or if none of the legacy
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

    private Manager manager;
    private long connection;
    private boolean keepReceiving;

    public PurpleSignal(long connection, String username) {
        this.connection = connection;
        this.keepReceiving = false;

        // stolen from signald/src/main/java/io/finn/signald/Main.java

        // Workaround for BKS truststore
        Security.insertProviderAt(new SecurityProvider(), 1);
        Security.addProvider(new BouncyCastleProvider());

        String data_path = getDefaultDataPath();
        System.out.println("Using data folder " + data_path);
        this.manager = new Manager(username, data_path);
        try {
            this.manager.init();
            System.out.println("purple-signal: Logged in with " + manager.getUsername());
        } catch (Exception e) {
            System.err.println("purple-signal: Error loading state file: " + e.getMessage());
        }
    }

    public void run() {
        System.out.println("purple-signal: STARTING TO RECEIVE");
        boolean returnOnTimeout = true; // it looks like setting this to false means "listen for new messages forever".
        // There seems to be a non-daemon thread to be involved somewhere as the Java VM
        // will not ever shut down.
        // TODO: Gain access to private MessagePipe, close it for immediate shutdown?
        long timeout = 60; // Seconds to wait for an incoming message. After the timeout occurred, a re-connect happens silently.
        // TODO: Find out how this affects what.
        boolean ignoreAttachments = true;
        try {
            while (this.keepReceiving) {
                this.manager.receiveMessages((long) (timeout * 1000), TimeUnit.MILLISECONDS, returnOnTimeout,
                    ignoreAttachments, this);
            }
        } catch (IOException | NotAGroupMemberException | GroupNotFoundException | AttachmentInvalidException e) {
            // TODO forward exception
        }
        System.out.println("purple-signal: RECEIVING DONE");
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
    }

    public static void main(String[] args) {
        PurpleSignal ps = new PurpleSignal(0, args[0]);
        ps.startReceiving();
    }

    @Override
    public void handleMessage(SignalServiceEnvelope envelope, SignalServiceContent content, Throwable exception) {
        System.out.println("purple-signal: RECEIVED SOMETHING!");
        // stolen from signald/src/main/java/io/finn/signald/MessageReceiver.java and
        // signal-cli/src/main/java/org/asamk/signal/JsonMessageEnvelope.java
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
                        who = Base64.encodeBytes(groupInfo.getGroupId()); // TODO: support groups
                                                                          // properly
                    }
                    if (dataMessage.getBody().isPresent()) {
                        String message = dataMessage.getBody().get();
                        System.out.println("purple-signal: CALLING NATIVE METHOD NOW");
                        handleMessageNatively(this.connection, who, message, timestamp);
                        // TODO: do not send receipt until handleMessageNatively returns successfully
                    }
                }
            }
            // TODO: support other message types
        }

    }

    static {
        System.loadLibrary("signal"); // TODO: change library name. this looks like it is asking for trouble already
    }

    public static native void handleMessageNatively(long connection, String who, String content, long timestamp);
}
