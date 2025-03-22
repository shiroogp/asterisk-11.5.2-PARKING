package biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.infra;

interface AsteriskStoragePort {
    void saveNewChannel(String channel, String uniqueId);

    void saveLocalBridge(String channel1, String channel2);

    void saveVarSet(String channel, String variable, String value);

    void logMaps();
}
