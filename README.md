# “DNS中继服务器”的实现

  设计一个DNS服务器程序，读入“IP地址-域名”对照表，当客户端查询域名对应的IP地址时，用域名检索该对照表，有三种可能检索结果：
  
  1) 检索结果：ip地址0.0.0.0，则向客户端返回“域名不存在”的报错消息（不良网站拦截功能）;    
  
  2) 检索结果：普通IP地址，则向客户端返回该地址（服务器功能）;    
  
  3) 表中未检到该域名，则向因特网DNS服务器发出查询，并将结果返给客户端（中继功能）    
    >考虑多个计算机上的客户端会同时查询，需要进行消息ID的转换
