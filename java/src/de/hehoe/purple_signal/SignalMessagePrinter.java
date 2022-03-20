package de.hehoe.purple_signal;

import java.util.List;

import org.asamk.signal.manager.groups.GroupUtils;
import org.whispersystems.signalservice.api.messages.SignalServiceAttachment;
import org.whispersystems.signalservice.api.messages.SignalServiceContent;
import org.whispersystems.signalservice.api.messages.SignalServiceDataMessage;
import org.whispersystems.signalservice.api.messages.SignalServiceEnvelope;
import org.whispersystems.signalservice.api.messages.SignalServiceGroup;
import org.whispersystems.signalservice.api.messages.SignalServiceGroup.Type;
import org.whispersystems.signalservice.api.messages.calls.SignalServiceCallMessage;
import org.whispersystems.signalservice.api.messages.SignalServiceGroupContext;
import org.whispersystems.signalservice.api.messages.SignalServiceGroupV2;
import org.whispersystems.signalservice.api.messages.multidevice.ReadMessage;
import org.whispersystems.signalservice.api.messages.multidevice.SentTranscriptMessage;
import org.whispersystems.signalservice.api.messages.multidevice.SignalServiceSyncMessage;
import org.whispersystems.signalservice.api.push.SignalServiceAddress;
import org.whispersystems.util.Base64;

public class SignalMessagePrinter {

	final static String printPrefix = "[signal] ";

	static public void printEnvelope(SignalServiceEnvelope envelope) {
		System.out.println(printPrefix + "SignalServiceEnvelope");
		long serverTimestamp = 0;// envelope.getServerTimestamp();
		System.out.println(printPrefix + "  ServerTimestamp: " + serverTimestamp);
		long timestamp = envelope.getTimestamp();
		System.out.println(printPrefix + "  Timestamp: " + timestamp);
		if (envelope.hasSourceUuid()) {
			String uuid = envelope.getSourceUuid().get();
			System.out.println(printPrefix + "  SourceUuid: " + uuid);
		}
		if (envelope.isPreKeySignalMessage()) {
			System.out.println(printPrefix + "  isPreKeySignalMessage");
		}
		if (envelope.isReceipt()) {
			System.out.println(printPrefix + "  isReceipt");
		}
		if (envelope.isSignalMessage()) {
			System.out.println(printPrefix + "  isSignalMessage");
		}
		if (envelope.isUnidentifiedSender()) {
			System.out.println(printPrefix + "  isUnidentifiedSender");
		}
	}

	static void printSignalServiceContent(SignalServiceContent content) {
		System.out.println(printPrefix + "SignalServiceContent");
		if (content.getCallMessage().isPresent()) {
			SignalServiceCallMessage callMessage = content.getCallMessage().get();
			System.out.println(printPrefix + "  CallMessage: " + callMessage.toString());
		}
		if (content.getDataMessage().isPresent()) {
			SignalServiceDataMessage dataMessage = content.getDataMessage().get();
			System.out.println(printPrefix + "  DataMessage");
			if (dataMessage.getAttachments().isPresent()) {
				List<SignalServiceAttachment> attachments = dataMessage.getAttachments().get();
				System.out.println(printPrefix + "    Attachments.size(): " + attachments.size());
			}
			if (dataMessage.getBody().isPresent()) {
				String body = dataMessage.getBody().get();
				System.out.println(printPrefix + "    Body: \"" + body + "\"");
			}
			if (dataMessage.getGroupContext().isPresent()) {
				SignalServiceGroupContext groupContext = dataMessage.getGroupContext().get();
				printGroupContext(groupContext, printPrefix + "    ");
			}
		}
		if (content.getReceiptMessage().isPresent()) {
			// SignalServiceReceiptMessage receiptMessage =
			// content.getReceiptMessage().get();
			System.out.println(printPrefix + "  ReceiptMessage: TODO");
		}
		long serverTimestamp = 0;// content.getServerTimestamp();
		System.out.println(printPrefix + "  serverTimestamp: " + serverTimestamp);
		if (content.getSyncMessage().isPresent()) {
			System.out.println(printPrefix + "  SyncMessage");
			SignalServiceSyncMessage syncMessage = content.getSyncMessage().get();
			if (syncMessage.getBlockedList().isPresent()) {
				System.out.println(printPrefix + "    BlockedList: TODO");
			}
			if (syncMessage.getConfiguration().isPresent()) {
				System.out.println(printPrefix + "    Configuration: TODO");
			}
			if (syncMessage.getContacts().isPresent()) {
				System.out.println(printPrefix + "    Contacts: TODO");
			}
			if (syncMessage.getFetchType().isPresent()) {
				System.out.println(printPrefix + "    FetchType: TODO");
			}
			if (syncMessage.getGroups().isPresent()) {
				System.out.println(printPrefix + "    Groups: TODO");
			}
			if (syncMessage.getKeys().isPresent()) {
				System.out.println(printPrefix + "    Keys: TODO");
			}
			if (syncMessage.getMessageRequestResponse().isPresent()) {
				System.out.println(printPrefix + "    MessageRequestResponse: TODO");
			}
			if (syncMessage.getRead().isPresent()) {
				System.out.println(printPrefix + "    Read:");
				List<ReadMessage> read = syncMessage.getRead().get();
				read.forEach((readMessage) -> {
					System.out.println(printPrefix + "      " + readMessage.toString());
				});
			}
			if (syncMessage.getRequest().isPresent()) {
				System.out.println(printPrefix + "    Request: TODO");
			}
			if (syncMessage.getSent().isPresent()) {
				System.out.println(printPrefix + "    Sent:");
				SentTranscriptMessage sent = syncMessage.getSent().get();
				if (sent.getDestination().isPresent()) {
					System.out.println(printPrefix + "      Destination:");
					SignalServiceAddress destination = sent.getDestination().get();
					String identifier = destination.getIdentifier();
					System.out.println(printPrefix + "        Identifier: " + identifier);
					if (destination.getNumber().isPresent()) {
						String number = destination.getNumber().get();
						System.out.println(printPrefix + "        Number: " + number);
					}
					/*
					if (destination.getRelay().isPresent()) {
						String relay = destination.getRelay().get();
						System.out.println(printPrefix + "        Relay: " + relay);
					}*/
				}
				long expirationStartTimestamp = sent.getExpirationStartTimestamp();
				if (expirationStartTimestamp > 0) {
					System.out.println(printPrefix + "      ExpirationStartTimestamp:" + expirationStartTimestamp);

				}
				SignalServiceDataMessage message = sent.getMessage();
				System.out.println(printPrefix + "      Message:");
				if (message.getAttachments().isPresent()) {
					System.out.println("        Attachments: TODO");
				}
				if (message.getBody().isPresent()) {
					String body = message.getBody().get();
					System.out.println(printPrefix + "        Body: \"" + body + "\"");
				}
				int expiresInSeconds = message.getExpiresInSeconds();
				if (expiresInSeconds > 0) {
					System.out.println(printPrefix + "        ExpiresInSeconds: " + expiresInSeconds);
				}
				if (message.getGroupContext().isPresent()) {
					SignalServiceGroupContext groupContext = message.getGroupContext().get();
					printGroupContext(groupContext, printPrefix + "        ");
				}
				System.out.println(printPrefix + "        Recipients: ");
				sent.getRecipients().forEach(serviceAdress -> {
					System.out.println(printPrefix + "          " + serviceAdress.getNumber().get());
				});
				System.out.println(printPrefix + "        timestamp: " + sent.getTimestamp());
				if (sent.isRecipientUpdate()) {
					System.out.println(printPrefix + "        isRecipientUpdate");
				}
			}
			if (syncMessage.getStickerPackOperations().isPresent()) {
				System.out.println(printPrefix + "    StickerPackOperations: TODO");
			}
			if (syncMessage.getVerified().isPresent()) {
				System.out.println(printPrefix + "    Verified: TODO");
			}
			if (syncMessage.getViewOnceOpen().isPresent()) {
				System.out.println(printPrefix + "    ViewOnceOpen: TODO");
			}
		}
		long timestamp = content.getTimestamp();
		System.out.println(printPrefix + "  timestamp: " + timestamp);
		if (content.getTypingMessage().isPresent()) {
			// SignalServiceTypingMessage typingMessage = content.getTypingMessage().get();
			System.out.println(printPrefix + "  TypingMessage: TODO");
		}
	}

	static public void printGroupContext(SignalServiceGroupContext groupContext, String prefix) {
		System.out.println(prefix + "GroupContext");
		if (groupContext.getGroupV1().isPresent()) {
			SignalServiceGroup group = groupContext.getGroupV1().get();
			System.out.println(prefix + "  GroupV1");
			if (group.getAvatar().isPresent()) {
				System.out.println(prefix + "    Avatar: TODO");
			}
			byte[] groupId = group.getGroupId();
			System.out.println(prefix + "    GroupId: " + Base64.encodeBytes(groupId));
			if (group.getMembers().isPresent()) {
				System.out.println(prefix + "    Members: TODO");
			}
			if (group.getName().isPresent()) {
				String name = group.getName().get();
				System.out.println(prefix + "    Name: " + name);
			}
			Type type = group.getType();
			System.out.println(prefix + "    Type: " + type.name());
		}
		if (groupContext.getGroupV2().isPresent()) {
			System.out.println(prefix + "  GroupV2");
			SignalServiceGroupV2 group = groupContext.getGroupV2().get();
			System.out.println(prefix + "    MasterKey: " + GroupUtils.getGroupId(groupContext).toBase64());
			int revision = group.getRevision();
			System.out.println(prefix + "    Revision: " + revision);
			// group.getSignedGroupChange();
		}
	}
}
