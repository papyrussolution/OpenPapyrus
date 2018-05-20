set wsdl2h=..\..\..\..\tools\gsoap-wsdl2h.exe -s -o
set soapcpp2=..\..\..\..\tools\gsoap-soapcpp2.exe -1 -i -n -C

rem $(SolutionDir)..\..\tools\gsoap-wsdl2h.exe -s -o vetisams.h http://api.vetrf.ru/schema/platform/services/2.0-RC-last/ams-mercury-g2b.service_v2.0_pilot.wsdl http://api.vetrf.ru/schema/platform/services/2.0-RC-last/EnterpriseService_v2.0_pilot.wsdl http://api.vetrf.ru/schema/platform/services/2.0-RC-last/ProductService_v2.0_pilot.wsdl
%wsdl2h% vetisams.h http://api.vetrf.ru/schema/platform/services/2.0-last/mercury-g2b.service_v2.0.wsdl http://api.vetrf.ru/schema/platform/services/ApplicationManagementService_v1.4_production.wsdl
%soapcpp2% -pvetisams vetisams.h
