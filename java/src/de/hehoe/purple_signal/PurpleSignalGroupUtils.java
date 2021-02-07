package de.hehoe.purple_signal;

import java.util.Set;
import java.util.stream.Collectors;

import org.asamk.signal.manager.Manager;
import org.whispersystems.signalservice.api.push.SignalServiceAddress;

public class PurpleSignalGroupUtils {
    public static Set<String> resolveMembers(Manager m, Set<SignalServiceAddress> addresses) {
        return addresses.stream()
                .map(m::resolveSignalServiceAddress)
                .map(SignalServiceAddress::getLegacyIdentifier)
                .collect(Collectors.toSet());
    }
}
