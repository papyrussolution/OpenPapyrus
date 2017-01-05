<?php
    require_once("..\\func.php");
    require_once("universe_htt_classes.php");
    require_once("universe_func.php");

    class UhttService
    {
        public function Login($login, $password)
        {
            /*
				Проверка авторизационных даных
				Подключение к Job-серверу через сокет
				Подключение Job-сервера к бд папируса
                
				Возврат:
					UhttOutStatus
				Status
					0 - ошибка подключения
					1 - удачное подключение
				Memo
					0 - Описание ошибки
					1 - OK
			*/

            // Получаем логин и пароль
            $login = iconv("UTF-8", "windows-1251", $login);
            $password = iconv("UTF-8", "windows-1251", $password);

            // Проверяем, может уже подключены
            if(isset($_COOKIE["SOAPClient"])) 
				$sid = $_COOKIE["SOAPClient"]; 
			else 
				$sid = "";
            if(strlen($sid) > 0) 
				session_id($sid);
            session_start();
            $logged = (int)SessionGetValue("logged", 0);
            if($logged > 0) {
                $values = new UhttOutStatus("0",iconv("windows-1251", "utf-8", "Уже авторизован."));
                return $values;
            }
            // Проверяем логин и пароль
            // !!!!!!
            
            // Подключились - необходимо сохранить параметры в сессии
            SessionSetValue("logged", 1);
           
            $values = new UhttOutStatus("1",iconv("windows-1251", "utf-8", session_id()));
            return $values;
        }
        // -----------------------------------------------------------------------------------------------------------
        // -----------------------------------------------------------------------------------------------------------
            
        public function Logout($dummy)
        {
            /*
				Отключение от сервера
                
				Возврат:
					UhttOutStatus
					Status
						0 - ошибка отключения
						1 - удачное отключение
					Memo
						0 - Описание ошибки
						1 - OK                
			*/
            
            // Проверяем подключение
            if(isset($_COOKIE["SOAPClient"])) $sid = $_COOKIE["SOAPClient"]; else $sid = "";
            if(strlen($sid) > 0) session_id($sid);
            session_start();
            $logged = (int)SessionGetValue("logged", 0);
            if($logged <= 0)
            {
                $values = new UhttOutStatus("0",iconv("windows-1251", "utf-8", "Не авторизован."));
                return $values;
            }
            
            // Закрываем сессию
            session_destroy();
            
            $values = new UhttOutStatus("1","OK");
            return $values;            
        }
        // -----------------------------------------------------------------------------------------------------------
        // -----------------------------------------------------------------------------------------------------------

        public function SelectObject($ObjSelectCriteria)
        {
            // Проверяем параметры
            if(!isset($ObjSelectCriteria->ObjName) || !isset($ObjSelectCriteria->ByCriteria))
                throw new SoapFault("0", iconv("windows-1251", "utf-8", "Не указаны обязательные параметры."));
            $obj = $ObjSelectCriteria->ObjName;
            $by = $ObjSelectCriteria->ByCriteria;
            
            // Проверяем подключение
            if(isset($_COOKIE["SOAPClient"])) 
				$sid = $_COOKIE["SOAPClient"]; else $sid = "";
            if(strlen($sid) > 0) 
				session_id($sid);
            session_start();
            $logged = (int)SessionGetValue("logged", 0);
            if($logged <= 0) {
                session_destroy();
                throw new SoapFault("0", iconv("windows-1251", "utf-8", "Не авторизован"));
            }
            
            // Подключаемся к Job серверу
            $handle = false;
            JobServer_Connect($handle);
            if($handle == false) 
				throw new SoapFault("0", iconv("windows-1251", "utf-8", "Ошибка подключения к Job серверу."));

            // Подключаемся к бд
            $logged = 0;
            $logged = PapyrusDB_Login($handle);
            if(!$logged) {
                if($handle != false) 
					JobServer_Quit($handle);
                throw new SoapFault("0", iconv("windows-1251", "utf-8", "Ошибка подключения к СУБД."));
            }
            
            // Пытаемся прочитать данные
            $obj = iconv("utf-8", "windows-1251", $obj);
            $by = iconv("utf-8", "windows-1251", $by);
            
            // Проверяем критерий
            if(!CheckSelectCriterion($obj, $by)) 
				throw new SoapFault("0", iconv("windows-1251", "utf-8", "Некорректно указаны критерии отбора."));
            
            $by = convert_cyr_string($by, "w", "a");
			if(strlen($by) == 0)
				$post = "SELECT ".$obj.endl;
			else
				$post = "SELECT ".$obj." by ".$by.endl;
            $buf = "";
            try {
                fwrite($handle, $post);
                $buf = fgets($handle);
            }
            catch(Exception $e) {
                // Отключаемся
                if($handle != false) 
					PapyrusDB_Logout($handle);
                if($handle != false) 
					JobServer_Quit($handle);
                throw new SoapFault("0",$e->getMessage());
            }

            // Отключаемся
            if($handle != false) 
				PapyrusDB_Logout($handle);
            if($handle != false) 
				JobServer_Quit($handle);
            
            // Прочитали чего-нибудь?
            if($buf[0] == "0") 
				throw new SoapFault("0",iconv("windows-1251", "utf-8", "По указанному запросу данных не найдено.".$post));
            
            // Разбираем ответ сервера
            $arr = array();
            preg_match_all("/<id>(\d+)<\/id><parentid>(\d+)<\/parentid><txt>(.*)<\/txt>/Ui", $buf, $arr, PREG_PATTERN_ORDER, 2);
            $lower = array_map('strtolower', $arr[3]);
            array_multisort($lower, $arr[3], SORT_STRING, SORT_ASC, $arr[2], $arr[1], $arr[0]);
            
            $values = array();
            for($i = 0; $i < count($arr[0]); $i++) {
                $values[] = new UhttCommObjItem($arr[1][$i], $arr[2][$i], iconv("windows-1251", "utf-8", $arr[3][$i]));
            }
                        
            return $values;
        }
        // -----------------------------------------------------------------------------------------------------------
        // -----------------------------------------------------------------------------------------------------------

        public function Get_Goods($id)
        {
            // Проверяем параметры
            if(!is_numeric($id)) 
				throw new SoapFault("0",iconv("windows-1251", "utf-8", "Не корректно указаны обязательные параметры."));
            
            // Проверяем подключение
            if(isset($_COOKIE["SOAPClient"])) 
				$sid = $_COOKIE["SOAPClient"]; else $sid = "";
            if(strlen($sid) > 0) 
				session_id($sid);
            session_start();
            $logged = (int)SessionGetValue("logged", 0);
            if($logged <= 0) {
                session_destroy();
                throw new SoapFault("0", iconv("windows-1251", "utf-8", "Не авторизован"));
            }
            
            // Подключаемся к Job серверу
            $handle = false;
            JobServer_Connect($handle);
            if($handle == false) 
				throw new SoapFault("0", iconv("windows-1251", "utf-8", "Ошибка подключения к Job серверу."));

            // Подключаемся к бд
            $logged = 0;
            $logged = PapyrusDB_Login($handle);
            if(!$logged) {
                if($handle != false) 
					JobServer_Quit($handle);
                throw new SoapFault("0", iconv("windows-1251", "utf-8", "Ошибка подключения к СУБД."));
            }
        
            $post = "SELECT DL600 UhttGoods by ID(".$id.")".endl;
            $buf = "";
            try
            {
                fwrite($handle, $post);
                $buf = fgets($handle);
            }
            catch(Exception $e)
            {
                // Отключаемся
                if($handle != false) 
					PapyrusDB_Logout($handle);
                if($handle != false) 
					JobServer_Quit($handle);
                throw new SoapFault("0",$e->getMessage());
            }

            // Отключаемся
            if($handle != false) 
				PapyrusDB_Logout($handle);
            if($handle != false) 
				JobServer_Quit($handle);
            
            // Прочитали чего-нибудь?
            if($buf[0] == "0") 
				throw new SoapFault("0",iconv("windows-1251", "utf-8", "По указанному ID данных не найдено.".$post));
            
            // Разбираем ответ сервера
            $buf[0] = "";
            $xmlstr = AddXMLEntities("UhttGoods").trim($buf);
            
            $xml = new SimpleXMLElement($xmlstr, LIBXML_DTDLOAD|LIBXML_DTDATTR);
            
            $values = new UhttGoods();
            $values->ID = (int)$xml->Head->ID;
            $values->ParentID = (int)$xml->Head->ParentID;
            $values->Name = (string)$xml->Head->Name;
            $values->UnitID = (int)$xml->Head->UnitID;
            $values->PhUnitID = (int)$xml->Head->PhUnitID;
            $values->PhUPerU = (float)$xml->Head->PhUPerU;
            $values->BrandID = (int)$xml->Head->BrandID;
            $values->ManufID = (int)$xml->Head->ManufID;
            $values->TaxGrpID = (int)$xml->Head->TaxGrpID;
            $values->Package = (float)$xml->Head->Package;
            // Список штрихкодов
            if(is_array($xml->xpath("//_BarcodeList")))
            {
                foreach($xml->xpath("//_BarcodeList") as $barcode) $values->BarcodeList[] = new UhttBarcode((string)$barcode->Code, (float)$barcode->Package); 
            }
            else    $values->BarcodeList[] = new UhttBarcode((string)$xml->_BarcodeList->Code, (float)$xml->_BarcodeList->Package);
            
            return $values;        
        }
		//
		//
		//
        public function Get_GoodsGroup($id)
        {
            // Проверяем параметры
            if(!is_numeric($id)) throw new SoapFault("0",iconv("windows-1251", "utf-8", "Не корректно указаны обязательные параметры."));
            
            // Проверяем подключение
            if(isset($_COOKIE["SOAPClient"])) $sid = $_COOKIE["SOAPClient"]; else $sid = "";
            if(strlen($sid) > 0) session_id($sid);
            session_start();
            $logged = (int)SessionGetValue("logged", 0);
            if($logged <= 0)
            {
                session_destroy();
                throw new SoapFault("0", iconv("windows-1251", "utf-8", "Не авторизован"));
            }
            
            // Подключаемся к Job серверу
            $handle = false;
            JobServer_Connect($handle);
            if($handle == false) throw new SoapFault("0", iconv("windows-1251", "utf-8", "Ошибка подключения к Job серверу."));

            // Подключаемся к бд
            $logged = 0;
            $logged = PapyrusDB_Login($handle);
            if(!$logged)
            {
                if($handle != false) JobServer_Quit($handle);
                throw new SoapFault("0", iconv("windows-1251", "utf-8", "Ошибка подключения к СУБД."));
            }
        
            $post = "SELECT DL600 GoodsGroup by ID(".$id.")".endl;
            $buf = "";
            try
            {
                fwrite($handle, $post);
                $buf = fgets($handle);
            }
            catch(Exception $e)
            {
                // Отключаемся
                if($handle != false) PapyrusDB_Logout($handle);
                if($handle != false) JobServer_Quit($handle);
                
                throw new SoapFault("0",$e->getMessage());
            }

            // Отключаемся
            if($handle != false) PapyrusDB_Logout($handle);
            if($handle != false) JobServer_Quit($handle);
            
            // Прочитали чего-нибудь?
            if($buf[0] == "0") throw new SoapFault("0", iconv("windows-1251", "utf-8", "По указанному ID данных не найдено.".$post));
            
            // Разбираем ответ сервера
            $buf[0] = "";
            $xmlstr = AddXMLEntities("GoodsGroup").trim($buf);
            
            $xml = new SimpleXMLElement($xmlstr, LIBXML_DTDLOAD|LIBXML_DTDATTR);
            
            $values = new UhttGoodsGroup();
            $values->ID = (int)$xml->Head->ID;
            $values->ParentID = (int)$xml->Head->ParentID;
            $values->Name = (string)$xml->Head->Name;
            $values->Code = (string)$xml->Head->Code;
            
            return $values;        
        }
		//
		//
		//
        public function Get_Brand($id)
        {
            // Проверяем параметры
            if(!is_numeric($id)) throw new SoapFault("0",iconv("windows-1251", "utf-8", "Не корректно указаны обязательные параметры."));
            
            // Проверяем подключение
            if(isset($_COOKIE["SOAPClient"])) $sid = $_COOKIE["SOAPClient"]; else $sid = "";
            if(strlen($sid) > 0) session_id($sid);
            session_start();
            $logged = (int)SessionGetValue("logged", 0);
            if($logged <= 0)
            {
                session_destroy();
                throw new SoapFault("0", iconv("windows-1251", "utf-8", "Не авторизован"));
            }
            
            // Подключаемся к Job серверу
            $handle = false;
            JobServer_Connect($handle);
            if($handle == false) throw new SoapFault("0", iconv("windows-1251", "utf-8", "Ошибка подключения к Job серверу."));

            // Подключаемся к бд
            $logged = 0;
            $logged = PapyrusDB_Login($handle);
            if(!$logged)
            {
                if($handle != false) JobServer_Quit($handle);
                throw new SoapFault("0", iconv("windows-1251", "utf-8", "Ошибка подключения к СУБД."));
            }
        
            $post = "SELECT DL600 Brand by ID(".$id.")".endl;
            $buf = "";
            try
            {
                fwrite($handle, $post);
                $buf = fgets($handle);
            }
            catch(Exception $e)
            {
                // Отключаемся
                if($handle != false) PapyrusDB_Logout($handle);
                if($handle != false) JobServer_Quit($handle);
                
                throw new SoapFault("0",$e->getMessage());
            }

            // Отключаемся
            if($handle != false) PapyrusDB_Logout($handle);
            if($handle != false) JobServer_Quit($handle);
            
            // Прочитали чего-нибудь?
            if($buf[0] == "0") throw new SoapFault("0", iconv("windows-1251", "utf-8", "По указанному ID данных не найдено.".$post));
            
            // Разбираем ответ сервера
            $buf[0] = "";
            $xmlstr = AddXMLEntities("Brand").trim($buf);
            
            $xml = new SimpleXMLElement($xmlstr, LIBXML_DTDLOAD|LIBXML_DTDATTR);
            
            $values = new UhttBrand();
            $values->ID = (int)$xml->Head->ID;
            $values->Name = (string)$xml->Head->Name;
            $values->Code = (string)$xml->Head->Code;
            $values->OwnerID = (int)$xml->Head->OwnerID;
            $values->GoodsGroupID = (int)$xml->Head->GoodsGrpID;
            
            return $values;        
        }		
		//
		//
		//
        public function Get_Person($id)
        {
            // Проверяем параметры
            if(!is_numeric($id)) throw new SoapFault("0",iconv("windows-1251", "utf-8", "Не корректно указаны обязательные параметры."));
            
            // Проверяем подключение
            if(isset($_COOKIE["SOAPClient"])) $sid = $_COOKIE["SOAPClient"]; else $sid = "";
            if(strlen($sid) > 0) session_id($sid);
            session_start();
            $logged = (int)SessionGetValue("logged", 0);
            if($logged <= 0)
            {
                session_destroy();
                throw new SoapFault("0", iconv("windows-1251", "utf-8", "Не авторизован"));
            }
            
            // Подключаемся к Job серверу
            $handle = false;
            JobServer_Connect($handle);
            if($handle == false) throw new SoapFault("0", iconv("windows-1251", "utf-8", "Ошибка подключения к Job серверу."));

            // Подключаемся к бд
            $logged = 0;
            $logged = PapyrusDB_Login($handle);
            if(!$logged)
            {
                if($handle != false) JobServer_Quit($handle);
                throw new SoapFault("0", iconv("windows-1251", "utf-8", "Ошибка подключения к СУБД."));
            }
        
            $post = "SELECT DL600 UhttPerson by ID(".$id.")".endl;
            $buf = "";
            try
            {
                fwrite($handle, $post);
                $buf = fgets($handle);
            }
            catch(Exception $e)
            {
                // Отключаемся
                if($handle != false) PapyrusDB_Logout($handle);
                if($handle != false) JobServer_Quit($handle);
                
                throw new SoapFault("0",$e->getMessage());
            }

            // Отключаемся
            if($handle != false) PapyrusDB_Logout($handle);
            if($handle != false) JobServer_Quit($handle);
            
            // Прочитали чего-нибудь?
            if($buf[0] == "0") throw new SoapFault("0", iconv("windows-1251", "utf-8", "По указанному ID данных не найдено.".$post));
            
            // Разбираем ответ сервера
            $buf[0] = "";
            $xmlstr = AddXMLEntities("UhttPerson").trim($buf);
            
            $xml = new SimpleXMLElement($xmlstr, LIBXML_DTDLOAD|LIBXML_DTDATTR);
            
            $values = new UhttPerson();
            $values->ID = (int)$xml->Head->ID;
            $values->Name = (string)$xml->Head->Name;
            $values->Code = (string)$xml->Head->Code;
            $values->KindNameList = array();
            $values->CategoryName = "";
            $values->StatusName = "";
            // Адрес и фактический адрес
            $values->Addr = new UhttLocation();
            $values->RAddr = new UhttLocation();
            if(is_array($xml->xpath("//_AddrList"))) {
                foreach($xml->xpath("//_AddrList") as $address) {
                    if((int)$address->LocKind == 1)
                        // Юридический адрес
                        CopyAddressFromXML($values->Addr, $address);
                    elseif((int)$address->LocKind == 2)
                        // Фактический адрес
                        CopyAddressFromXML($values->RAddr, $address);                    
                }
            }
            else if(isset($xml->_AddressList))
				if((int)$xml->_AddressList->LocKind == 1)
					// Юридический адрес
					CopyAddressFromXML($values->Addr, $xml->_AddressList);
				elseif((int)$xml->_AddressList->LocKind == 2)
					// Фактический адрес
					CopyAddressFromXML($values->RAddr, $xml->_AddressList);
            
            // Список телефонов
            if(is_array($xml->xpath("//_PhoneList")))
            {
                foreach($xml->xpath("//_PhoneList") as $phone) 
					$values->PhoneList[] = (string)$phone->Code; 
            }
            else    
				$values->PhoneList[] = (string)$xml->_PhoneList->Code;
            
            // Список электронных почтовых адресов
            if(is_array($xml->xpath("//_EMailList")))
            {
                foreach($xml->xpath("//_EMailList") as $email) 
					$values->EmailList[] = (string)$email->Code; 
            }
            else if(isset($xml->_EmailList)) 
				$values->EmailList[] = (string)$xml->_EmailList->Code;
            
            // URL
            if(isset($xml->_UrlList))
                if(is_array($xml->xpath("//_UrlList"))) {
                    $UrlList = $xml->xpath("//_UrlList");
                    $values->URL = (string)($UrlList[0]->Code); 
                }
                else if(isset($xml->_UrlList)) 
					$values->URL = (string)$xml->_UrlList->Code;
            
            // Список регистров
            if(is_array($xml->xpath("//_RegisterList"))) {
                foreach($xml->xpath("//_RegisterList") as $reg) {
                    $ID = (int)$reg->RegID;
                    $TypeID = (int)$reg->RegTypeID;
                    $PersonID = (int)$reg->PersonID;
                    $OrgID = (int)$reg->RegOrgID;
                    $Dt = strtotime($reg->RegDt); // NULL ?
                    $Expiry = strtotime($reg->RegExpiry); // NULL ?
                    $Serial = (string)$reg->RegSerial;
                    $Number = (string)$reg->RegNumber;
                    $values->RegisterList[] = new UhttPersonRegister($ID, $TypeID, $PersonID, $OrgID, $Dt, $Expiry, $Serial, $Number);
                }
            }
            else if(isset($xml->_RegisterList)) {
				$ID = (int)$xml->_RegisterList->RegID;
				$TypeID = (int)$xml->_RegisterList->RegTypeID;
				$PersonID = (int)$xml->_RegisterList->PersonID;
				$OrgID = (int)$xml->_RegisterList->RegOrgID;
				$Dt = strtotime($xml->_RegisterList->RegDt); // NULL ?
				$Expiry = strtotime($xml->_RegisterList->RegExpiry); // NULL ?
				$Serial = (string)$xml->_RegisterList->RegSerial;
				$Number = (string)$xml->_RegisterList->Number;
				$values->RegisterList[] = new UhttPersonRegister($ID, $TypeID, $PersonID, $OrgID, $Dt, $Expiry, $Serial, $Number);
			}
            
            return $values;        
        }
		//
		//
		//
        public function Get_Currency($id)
        {
            // Проверяем параметры
            if(!is_numeric($id)) throw new SoapFault("0",iconv("windows-1251", "utf-8", "Не корректно указаны обязательные параметры."));
            
            // Проверяем подключение
            if(isset($_COOKIE["SOAPClient"])) $sid = $_COOKIE["SOAPClient"]; else $sid = "";
            if(strlen($sid) > 0) session_id($sid);
            session_start();
            $logged = (int)SessionGetValue("logged", 0);
            if($logged <= 0)
            {
                session_destroy();
                throw new SoapFault("0", iconv("windows-1251", "utf-8", "Не авторизован"));
            }
            
            // Подключаемся к Job серверу
            $handle = false;
            JobServer_Connect($handle);
            if($handle == false) throw new SoapFault("0", iconv("windows-1251", "utf-8", "Ошибка подключения к Job серверу."));

            // Подключаемся к бд
            $logged = 0;
            $logged = PapyrusDB_Login($handle);
            if(!$logged)
            {
                if($handle != false) JobServer_Quit($handle);
                throw new SoapFault("0", iconv("windows-1251", "utf-8", "Ошибка подключения к СУБД."));
            }
        
            $post = "SELECT DL600 Currency by ID(".$id.")".endl;
            $buf = "";
            try
            {
                fwrite($handle, $post);
                $buf = fgets($handle);
            }
            catch(Exception $e)
            {
                // Отключаемся
                if($handle != false) PapyrusDB_Logout($handle);
                if($handle != false) JobServer_Quit($handle);
                
                throw new SoapFault("0",$e->getMessage());
            }

            // Отключаемся
            if($handle != false) PapyrusDB_Logout($handle);
            if($handle != false) JobServer_Quit($handle);
            
            // Прочитали чего-нибудь?
            if($buf[0] == "0") throw new SoapFault("0", iconv("windows-1251", "utf-8", "По указанному ID данных не найдено.".$post));
            
            // Разбираем ответ сервера
            $buf[0] = "";
            $xmlstr = AddXMLEntities("Currency").trim($buf);
            
            $xml = new SimpleXMLElement($xmlstr, LIBXML_DTDLOAD|LIBXML_DTDATTR);
            
            $values = new UhttCurrency();
            $values->ID = (int)$xml->Head->ID;
            $values->Name = (string)$xml->Head->Name;
            $values->Code = (string)$xml->Head->Symb;
            $values->DigitCode = (int)$xml->Head->DigitCode;
            
            return $values;        
        }
		//
		//
		//
        public function Get_PersonKind($id)
        {
            // Проверяем параметры
            if(!is_numeric($id)) throw new SoapFault("0",iconv("windows-1251", "utf-8", "Не корректно указаны обязательные параметры."));
            
            // Проверяем подключение
            if(isset($_COOKIE["SOAPClient"])) $sid = $_COOKIE["SOAPClient"]; else $sid = "";
            if(strlen($sid) > 0) session_id($sid);
            session_start();
            $logged = (int)SessionGetValue("logged", 0);
            if($logged <= 0)
            {
                session_destroy();
                throw new SoapFault("0", iconv("windows-1251", "utf-8", "Не авторизован"));
            }
            
            // Подключаемся к Job серверу
            $handle = false;
            JobServer_Connect($handle);
            if($handle == false) throw new SoapFault("0", iconv("windows-1251", "utf-8", "Ошибка подключения к Job серверу."));

            // Подключаемся к бд
            $logged = 0;
            $logged = PapyrusDB_Login($handle);
            if(!$logged)
            {
                if($handle != false) JobServer_Quit($handle);
                throw new SoapFault("0", iconv("windows-1251", "utf-8", "Ошибка подключения к СУБД."));
            }
        
            $post = "SELECT DL600 PersonKind by ID(".$id.")".endl;
            $buf = "";
            try
            {
                fwrite($handle, $post);
                $buf = fgets($handle);
            }
            catch(Exception $e)
            {
                // Отключаемся
                if($handle != false) PapyrusDB_Logout($handle);
                if($handle != false) JobServer_Quit($handle);
                
                throw new SoapFault("0",$e->getMessage());
            }

            // Отключаемся
            if($handle != false) PapyrusDB_Logout($handle);
            if($handle != false) JobServer_Quit($handle);
            
            // Прочитали чего-нибудь?
            if($buf[0] == "0") throw new SoapFault("0", iconv("windows-1251", "utf-8", "По указанному ID данных не найдено.".$post));
            
            // Разбираем ответ сервера
            $buf[0] = "";
            $xmlstr = AddXMLEntities("PersonKind").trim($buf);
            
            $xml = new SimpleXMLElement($xmlstr, LIBXML_DTDLOAD|LIBXML_DTDATTR);
            
            $values = new UhttPersonKind();
            $values->ID = (int)$xml->Head->ID;
            $values->Name = (string)$xml->Head->Name;
            $values->Code = (string)$xml->Head->Code;
            
            return $values;        
        }
		//
		//
		//
        public function Get_PersonCategory($id)
        {
            // Проверяем параметры
            if(!is_numeric($id)) throw new SoapFault("0",iconv("windows-1251", "utf-8", "Не корректно указаны обязательные параметры."));
            
            // Проверяем подключение
            if(isset($_COOKIE["SOAPClient"])) $sid = $_COOKIE["SOAPClient"]; else $sid = "";
            if(strlen($sid) > 0) session_id($sid);
            session_start();
            $logged = (int)SessionGetValue("logged", 0);
            if($logged <= 0)
            {
                session_destroy();
                throw new SoapFault("0", iconv("windows-1251", "utf-8", "Не авторизован"));
            }
            
            // Подключаемся к Job серверу
            $handle = false;
            JobServer_Connect($handle);
            if($handle == false) throw new SoapFault("0", iconv("windows-1251", "utf-8", "Ошибка подключения к Job серверу."));

            // Подключаемся к бд
            $logged = 0;
            $logged = PapyrusDB_Login($handle);
            if(!$logged)
            {
                if($handle != false) JobServer_Quit($handle);
                throw new SoapFault("0", iconv("windows-1251", "utf-8", "Ошибка подключения к СУБД."));
            }
        
            $post = "SELECT DL600 PersonCat by ID(".$id.")".endl;
            $buf = "";
            try
            {
                fwrite($handle, $post);
                $buf = fgets($handle);
            }
            catch(Exception $e)
            {
                // Отключаемся
                if($handle != false) PapyrusDB_Logout($handle);
                if($handle != false) JobServer_Quit($handle);
                
                throw new SoapFault("0",$e->getMessage());
            }

            // Отключаемся
            if($handle != false) PapyrusDB_Logout($handle);
            if($handle != false) JobServer_Quit($handle);
            
            // Прочитали чего-нибудь?
            if($buf[0] == "0") throw new SoapFault("0", iconv("windows-1251", "utf-8", "По указанному ID данных не найдено.".$post));
            
            // Разбираем ответ сервера
            $buf[0] = "";
            $xmlstr = AddXMLEntities("PersonCat").trim($buf);
            
            $xml = new SimpleXMLElement($xmlstr, LIBXML_DTDLOAD|LIBXML_DTDATTR);
            
            $values = new UhttPersonCategory();
            $values->ID = (int)$xml->Head->ID;
            $values->Name = (string)$xml->Head->Name;
            $values->Code = (string)$xml->Head->Code;
            
            return $values;        
        }
		//
		//
		//
        public function Get_RegisterType($id)
        {
            // Проверяем параметры
            if(!is_numeric($id)) 
				throw new SoapFault("0",iconv("windows-1251", "utf-8", "Не корректно указаны обязательные параметры."));
            
            // Проверяем подключение
            if(isset($_COOKIE["SOAPClient"])) 
				$sid = $_COOKIE["SOAPClient"]; 
			else 
				$sid = "";
            if(strlen($sid) > 0) 
				session_id($sid);
            session_start();
            $logged = (int)SessionGetValue("logged", 0);
            if($logged <= 0) {
                session_destroy();
                throw new SoapFault("0", iconv("windows-1251", "utf-8", "Не авторизован"));
            }
            
            // Подключаемся к Job серверу
            $handle = false;
            JobServer_Connect($handle);
            if($handle == false) throw new SoapFault("0", iconv("windows-1251", "utf-8", "Ошибка подключения к Job серверу."));

            // Подключаемся к бд
            $logged = 0;
            $logged = PapyrusDB_Login($handle);
            if(!$logged) {
                if($handle != false) JobServer_Quit($handle);
                throw new SoapFault("0", iconv("windows-1251", "utf-8", "Ошибка подключения к СУБД."));
            }
        
            $post = "SELECT DL600 RegisterType by ID(".$id.")".endl;
            $buf = "";
            try
            {
                fwrite($handle, $post);
                $buf = fgets($handle);
            }
            catch(Exception $e)
            {
                // Отключаемся
                if($handle != false) PapyrusDB_Logout($handle);
                if($handle != false) JobServer_Quit($handle);
                
                throw new SoapFault("0",$e->getMessage());
            }

            // Отключаемся
            if($handle != false) 
				PapyrusDB_Logout($handle);
            if($handle != false) 
				JobServer_Quit($handle);
            
            // Прочитали чего-нибудь?
            if($buf[0] == "0") 
				throw new SoapFault("0", iconv("windows-1251", "utf-8", "По указанному ID данных не найдено.".$post));
            
            // Разбираем ответ сервера
            $buf[0] = "";
            $xmlstr = AddXMLEntities("RegisterType").trim($buf);
            $xml = new SimpleXMLElement($xmlstr, LIBXML_DTDLOAD|LIBXML_DTDATTR);
            $values = new UhttPersonRegisterType();
            $values->ID = (int)$xml->Head->ID;
            $values->Name = (string)$xml->Head->Name;
            $values->Code = (string)$xml->Head->Symb;
            
            return $values;        
        }
    }

    ini_set("soap.wsdl_cache_enabled", "0"); // отключаем кэширование WSDL     
    $server = new SoapServer("universe_htt.wsdl"); 
    $server->setClass("UhttService"); 
    $server->handle();
?>