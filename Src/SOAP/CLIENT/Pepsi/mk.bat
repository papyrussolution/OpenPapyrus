set wsdl2h=..\..\..\..\tools\gsoap-wsdl2h.exe -s -o
set soapcpp2=..\..\..\..\tools\gsoap-soapcpp2.exe -1 -i -n -C

%wsdl2h%   pepsiSoap.h accountingtransfer.asmx.xml
%soapcpp2% -ppepsiSoap pepsiSoap.h
