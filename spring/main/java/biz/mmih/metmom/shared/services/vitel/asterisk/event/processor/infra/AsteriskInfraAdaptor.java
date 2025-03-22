package biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.infra;

import biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.core.api.AsteriskInfraPort;
import lombok.SneakyThrows;
import lombok.extern.slf4j.Slf4j;
import org.asteriskjava.live.AsteriskServer;
import org.asteriskjava.live.AsteriskServerListener;
import org.asteriskjava.manager.ManagerEventListener;
import org.asteriskjava.manager.ManagerEventListenerProxy;
import org.asteriskjava.manager.action.ManagerAction;
import org.asteriskjava.manager.response.ManagerResponse;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.core.env.Environment;
import org.springframework.stereotype.Component;

@Component
@Slf4j
class AsteriskInfraAdaptor implements AsteriskInfraPort {

    private final AsteriskServer asteriskServer;
    private final AsteriskStoragePort asteriskStoragePort;
    @Value("${api.host.baseurl}")
    private String baseURL;

    public AsteriskInfraAdaptor(AsteriskServer asteriskServer, AsteriskStoragePort asteriskStoragePort) {
        this.asteriskServer = asteriskServer;
        this.asteriskStoragePort = asteriskStoragePort;

    }

    @Override
    public void setAsteriskServerEventListener(ManagerEventListener eventListener) {
        this.asteriskServer.addChainListener(new ManagerEventListenerProxy(eventListener));
    }

    @Override
    public void setAsteriskServerListener(AsteriskServerListener serverListenerListener) {
        this.asteriskServer.addAsteriskServerListener(serverListenerListener);
    }

    @SneakyThrows
    @Override
    public ManagerResponse sendAction(ManagerAction action) {
        return this.asteriskServer.getManagerConnection().sendAction(action);
    }

    @Override
    public void saveNewChannel(String channel, String uniqueId) {
        asteriskStoragePort.saveNewChannel(channel, uniqueId);
    }

    @Override
    public void saveLocalBridge(String channel1, String channel2) {
        asteriskStoragePort.saveLocalBridge(channel1, channel2);
    }

    @Override
    public void saveVarSet(String channel, String variable, String value) {
        asteriskStoragePort.saveVarSet(channel, variable, value);
    }

    @Override
    public void logStorageMaps() {
        asteriskStoragePort.logMaps();
    }
}
