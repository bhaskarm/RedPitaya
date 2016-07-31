%% First set the ethernet from public network to private using the 
%% administrator level powershell:
%% Set-NetConnectionProfile -interfacealias Ethernet -NetworkCategory Private
%% For security windows 10 does not allow p2p gigabit ethernet to be private by default

  pkg load sockets;
  rcv_sock = socket(AF_INET, SOCK_DGRAM, 0);
  printf("Tryingi to socket 5001\n");
  bind(rcv_sock, 5001);
  printf("Bound to socket 5001\n");
  %a = listen(rcv_sock, 1);
  printf("Listening to socket\n");
  while true
    [data, count] = recv(rcv_sock, 1000);
    printf("Recvd data from socket\n");
    printf("Recvd %s at time :%s", char(data), strftime("%Y-%m-%d %r", localtime(time())))
    %disconnect(client);
  end
