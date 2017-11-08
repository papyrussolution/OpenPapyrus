set wsdl2h=..\..\..\..\tools\gsoap-wsdl2h.exe -s -o
set soapcpp2=..\..\..\..\tools\gsoap-soapcpp2.exe -1 -i -n -C

rem %wsdl2h%   mvu.h mercury-vu.service_v2.0.wsdl
rem %soapcpp2% -pmvu mvu.h

rem %wsdl2h%   mg2b.h mercury_g2b_applications_v2.0.xsd
%wsdl2h%   mg2b.h mercury-g2b.service_v2.0.wsdl
%soapcpp2% -pmg2b mg2b.h

rem %wsdl2h%   mapp.h ApplicationManagementService_v1.4_pilot.wsdl
rem %soapcpp2% -pmapp mapp.h


