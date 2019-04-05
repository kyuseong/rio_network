# rio_network


## server code


```cpp

  m_Network = CreateNetwork(L"DummyServer", 10000, 2, 2);
	
	m_Network->Start([](auto& Net, auto ID){
		return new DummySession(Net, ID);
	});
  m_Network->StartAcceptor(L"", 10031, true);
  
  
  ...
  
  
  
  
  m_Network->Shutdown();
  delete m_Network;
  

```



## client code



```cpp

  m_NetworkClient = CreateNetwork( L"DummyClient", 1000, 1, 1);

	m_NetworkClient->Start([](Network& Net, unsigned short SID)
	{
		return new ClientNet(Net, SID);
	});
  
  auto ClientNet = m_NetworkClient->ConnectSession(m_IP.c_str(), m_Port);
  ...
    
  
  m_Network->Shutdown();
  delete m_Network;
  

```

