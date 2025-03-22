package biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.infra;

import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Component;

import java.util.HashMap;
import java.util.Map;

@Slf4j
@Component
class AsteriskMemoryStorageAdaptor implements AsteriskStoragePort {

    final Map<String, String> channelMap = new HashMap<>(0);
    final Map<String, String> localBridgeMap = new HashMap<>(0);
    final Map<String, Map<String, String>> varSetMap = new HashMap<>(0);

    @Override
    public void saveNewChannel(String channel, String uniqueId) {

    }

    @Override
    public void saveLocalBridge(String channel1, String channel2) {

    }

    @Override
    public void saveVarSet(String channel, String variable, String value) {

    }

    @Override
    public void logMaps() {
        log.info("Channels: {} bridges {} vars {}", channelMap, localBridgeMap, varSetMap);
    }

}
