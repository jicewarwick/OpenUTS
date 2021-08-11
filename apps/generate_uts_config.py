#!/usr/bin/env python

import pandas as pd
import json
import sys


if __name__ == '__main__':
    loc = 'config.xlsx' if len(sys.argv) == 1 else sys.argv[1]
    output_config = {}

    uts_config = pd.read_excel(loc, sheet_name='UnifiedTradingSystemConfig')
    for _, row in uts_config.iterrows():
        output_config[row[0]] = row[1]

    output_config['md_server'] = {}
    md_server = pd.read_excel(loc, sheet_name='md_server', dtype=object)
    for _, row in md_server.iterrows():
        output_config['md_server'][row.broker_name] = row.md_addr

    brokers = pd.read_excel(loc, sheet_name='brokers', dtype=object)
    brokers['query_rate_per_second'] = brokers['query_rate_per_second'].astype(int)
    output_config['brokers'] = brokers.to_dict('record')

    accounts = pd.read_excel(loc, sheet_name='accounts', dtype=object)
    accounts['password'] = accounts['password'].astype(str)
    accounts['enable'] = accounts['enable'].astype(bool)
    output_config['accounts'] = []
    accounts_info = accounts.to_dict('record')
    for tmp in accounts_info:
        for key, val in tmp.items():
            if isinstance(val, str):
                tmp[key] = val.replace('\'', '')
        output_config['accounts'].append(tmp)

    with open('config.json', 'w', encoding='utf-8') as f:
        json.dump(output_config, f, indent=4, ensure_ascii=False)

