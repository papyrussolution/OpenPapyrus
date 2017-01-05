set gsoap_path=d:\papyrus\tools\gSOAP
%gsoap_path%\wsdl2h.exe -t%gsoap_path%\ws\typemap.dat -s -f -x universe_htt.wsdl
rem @pause
%gsoap_path%\soapcpp2.exe -C -L -x universe_htt.h
