import datetime as dt
import sqlite3
from io import StringIO
from typing import Sequence, Union

import pandas as pd
import requests


def get_current_cffex_contracts(products: Union[str, Sequence[str]]):
    """Get IC, IF, IH, IO contracts from CFFEX"""
    today = dt.datetime.today()
    url = f'http://www.cffex.com.cn/sj/jycs/{today.strftime("%Y%m")}/{today.strftime("%d")}/{today.strftime("%Y%m%d")}_1.csv'
    rsp = requests.get(url)
    rsp.encoding = 'gbk'
    data = pd.read_csv(StringIO(rsp.text), skiprows=1)
    data = data.loc[:, ['合约代码', '最后交易日']].set_index('合约代码')
    data['最后交易日'] = data['最后交易日'].astype('str')
    tmp = data.to_dict()
    ret = tmp['最后交易日']
    if isinstance(products, str):
        products = [products]
    kept_keys = []
    for key in ret.keys():
        for product in products:
            if key.startswith(product):
                kept_keys.append(key)
                break
    ret = {key: value for key, value in ret.items() if key in kept_keys}
    return ret


if __name__ == '__main__':
    conn = sqlite3.connect('db.sqlite3')

    contracts = get_current_cffex_contracts(['IF', 'IO'])
    contracts_df = pd.DataFrame.from_dict(contracts, orient='index').reset_index()
    contracts_df.columns = ['ticker', 'end_date']
    contracts_df.to_sql('cffex_contracts', conn, if_exists='replace')
