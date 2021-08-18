import os
import sqlite3
import sys

import pandas as pd
import xmltodict


def parse_file(loc):
	""" 从快期的 'broker.xml' 中提取期货公司的 `CTP` 交易和行情服务器地址

	该文件可中 %APPDATA%/Q{VERSION}????/{DATETIME}/ 中找到
	"""
	with open(loc, 'r', encoding='gb2312') as f:
		txt = f.read()
	info = xmltodict.parse(txt)
	brokers = info['root']['broker']
	trading_server = {}
	md_server = {}
	broker_id = {}

	for broker in brokers:
		broker_name = broker['@BrokerName']
		if broker_name[0].isupper():
			broker_name = broker_name[1:]
		broker_id[broker_name] = broker['@BrokerID']
		server_info = broker['Servers']['Server']
		if not isinstance(server_info, list):
			server_info = [server_info]
		trading_server[broker_name] = {}
		for info in server_info:
			server_naming = info['Name']
			address_info = info['Trading']['item']
			if not isinstance(address_info, list):
				address_info = [address_info]
			trading_server[broker_name][server_naming] = ','.join(address_info)

			if 'MarKetData' in info.keys():
				info['MarketData'] = info['MarKetData']
			address_info = info['MarketData']['item']
			if not isinstance(address_info, list):
				address_info = [address_info]
			for i, server in enumerate(address_info):
				name = f'{broker_name}{server_naming}{i + 1}'
				md_server[name] = server

	return broker_id, trading_server, md_server


if __name__ == '__main__':
	if len(sys.argv) > 1:
		loc = sys.argv[1]
	else:
		app_data_dir = os.getenv('APPDATA')
		l0_folder = os.listdir(app_data_dir)
		l0_folder = [folder for folder in l0_folder if 'shinny' in folder]
		tmp = os.listdir(os.path.join(app_data_dir, l0_folder[-1]))
		l2_folder = [folder for folder in tmp if folder.startswith('20')]
		loc = os.path.join(app_data_dir, l0_folder[-1], l2_folder[-1], 'broker.xml')
	if not os.path.exists(loc):
		raise RuntimeError('Cannot find broker.xml!')
	if len(sys.argv) > 2:
		db_loc = sys.argv[2]
	else:
		db_loc = 'db.sqlite3'

	broker_id, trading_server, md_server = parse_file(loc)
	conn = sqlite3.connect(db_loc)

	md_df = pd.DataFrame.from_dict(md_server, orient='index').reset_index()
	md_df.columns = ['name', 'address']
	md_df.to_sql('ctp_md_server', conn, if_exists='replace')

	trade_server_df = pd.DataFrame.from_dict(trading_server, orient='index').stack().reset_index()
	trade_server_df.columns = ['broker', 'server_name', 'trade_server']

	broker_df = pd.DataFrame.from_dict(broker_id, orient='index').reset_index()
	broker_df.columns = ['broker', 'broker_id']

	trade_info = pd.merge(trade_server_df, broker_df, on='broker')
	trade_info['AppID'] = ''
	trade_info['AuthCode'] = ''
	trade_info.to_sql('ctp_trade_server', conn, if_exists='replace')
