set wsdl2h=..\..\..\..\tools\gsoap-wsdl2h.exe -s -o
set soapcpp2=..\..\..\..\tools\gsoap-soapcpp2.exe -1 -i -n -C

%wsdl2h%   korus.h EDIservice.asmx.xml
%soapcpp2% -pkorus korus.h
