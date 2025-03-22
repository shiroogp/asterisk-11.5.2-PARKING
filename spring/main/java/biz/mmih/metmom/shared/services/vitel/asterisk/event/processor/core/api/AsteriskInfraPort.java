package biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.core.api;

import org.asteriskjava.live.AsteriskServerListener;
import org.asteriskjava.manager.ManagerEventListener;
import org.asteriskjava.manager.action.ManagerAction;
import org.asteriskjava.manager.response.ManagerResponse;

public interface AsteriskInfraPort {

    void setAsteriskServerEventListener(ManagerEventListener eventListener);

    void setAsteriskServerListener(AsteriskServerListener serverListenerListener);

    ManagerResponse sendAction(ManagerAction action);

    void saveNewChannel(String channel, String uniqueId);

    void saveLocalBridge(String channel1, String channel2);

    void saveVarSet(String channel, String variable, String value);

    void logStorageMaps();
}
