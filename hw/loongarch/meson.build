loongarch_ss = ss.source_set()
loongarch_ss.add(when: 'CONFIG_LOONGARCH_VIRT', if_true: files('loongson3.c'))
loongarch_ss.add(when: 'CONFIG_FW_CFG_LOONGARCH', if_true: files('fw_cfg.c'))

hw_arch += {'loongarch': loongarch_ss}
