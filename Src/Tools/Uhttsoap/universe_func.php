<?php
   
    /*
        Функция возвращает XML заголовок и описание сущностей, возвращаемых папирусом
        $root - корневой элемент в описании XML
        
        Используется для php расширения SimpleXML
    */
    function AddXMLEntities($root)
    {
        $entities = "<!DOCTYPE ".$root." [<!ENTITY amp '&#38;#38;'><!ENTITY exclam '&#33;'><!ENTITY sharp '&#35;'><!ENTITY pct '&#37;'><!ENTITY apostr '&#39;'><!ENTITY comma '&#44;'><!ENTITY dot '&#46;'><!ENTITY fwsl '&#47;'><!ENTITY lt '&#38;#60;'><!ENTITY eq '&#61;'><!ENTITY gt '&#62;'><!ENTITY ques '&#63;'><!ENTITY lsq '&#91;'><!ENTITY bksl '&#92;'><!ENTITY rsq '&#93;'>]>";
        return "<?xml version='1.0' encoding='windows-1251'?>".$entities;
    }

    /*
        Функция копирует данные из UhttXML - XML представления строки в объект
        UhttLocation
        
        Используется для сокращения основного кода
    */
    function CopyAddressFromXML(&$UhttLocation, $UhttXML)
    {
        $UhttLocation->LocID = (int)$UhttXML->LocID;
        $UhttLocation->LocKind = (int)$UhttXML->LocKind;
        $UhttLocation->LocCode = (string)$UhttXML->LocCode;
        $UhttLocation->LocName = (string)$UhttXML->LocName;
        $UhttLocation->CountryID = (int)$UhttXML->CountryID;
        $UhttLocation->Country = (string)$UhttXML->Country;
        $UhttLocation->CityID = (int)$UhttXML->CityID;
        $UhttLocation->City = (string)$UhttXML->City;
        $UhttLocation->ZIP = (string)$UhttXML->ZIP;
        $UhttLocation->Address = (string)$UhttXML->Address;
        $UhttLocation->Latitude = (float)$UhttXML->Latitude;
        $UhttLocation->Longitude = (float)$UhttXML->Longitude;
    }
    
    /*
        Функция проверяет переданные имя объекта и критерий выбора на соответствие
        правилам. Все некорректно оформленные критерии отбрасываются и не участвуют
        в проверке. Корректно оформленным считается запись вида
        критерий(условие)
        в случае, если все найденные критерии корректны - изменяет входящую
        переменную ByCriteria, оставля только корректные критерии.
        
        Строки должны быть в кодировке windows-1251
        Возвращает true в случае успешной проверки
    */
    function CheckSelectCriterion(&$ObjName, &$ByCriteria)
    {
        // Массив разрешенных объектов и соответствующих им критериев
        $UhttObjects = array
        (
            "goods" => array
            (
                "id" => "",
                "name" => "",
                "subname" => "",
                "parent.id" => "",
                "parent.code" => "",
                "parent.name" => "",
                "brand.id" => "",
                "brand.name" => "",
                "manuf.id" => "",
                "manuf.name" => "",
                "manuf.code" => "",
                "code" => "",
                "barcode" => "",
                "arcode" => ""
            ),
            "goodsgroup" => array
            (
                "id" => "",
                "name" => "",
                "subname" => "",
                "code" => "",
                "parent.id" => "",
                "parent.code" => "",
                "parent.name" => ""
            ),
            "brand" => array
            (
                "id" => "",
                "name" => "",
                "subname" => "",
                "owner.id" => "",
                "owner.code" => "",
                "owner.name" => ""
            ),
            "personkind" => array
            (
                "id" => "",
                "name" => "",
                "code" => ""
            ),
            "person" => array
            (
                "id" => "",
                "name" => "",
                "subname" => "",
                "kind.id" => "",
                "kind.name" => "",
                "kind.code" => "",
                "register.def" => "",
                "register.id" => "",
                "register.code" => ""
            ),
            "city" => array
            (
                "id" => "",
                "name" => "",
                "code" => "",
                "parent.id" => "",
                "parent.code" => ""
            ),
            "country" => array
            (
                "id" => "",
                "name" => "",
                "code" => "",
                "parent.id" => "",
                "parent.code" => ""
            )
        );
        
        // Обрезаем пробелы
        $ObjName = trim($ObjName);
        $ByCriteria = trim($ByCriteria);
        
        $valid = true;
        if(!isset($UhttObjects[strtolower($ObjName)])) 
			$valid = false;
        elseif(strlen($ByCriteria) > 0) {
            // Разбираем критерий
            $arr = array();
            preg_match_all("/(.+)\(.+\)/Ui", $ByCriteria, $arr, PREG_PATTERN_ORDER);
            $c = count($arr[1]);
            if($c == 0) 
				$valid = false;
            else {
                // Проверяем все критерии на корректность
                $ByNew = "";
                for($i = 0; $i < $c; $i++) {
                    $ByNew .= $arr[0][$i];
                    if(!isset($UhttObjects[$ObjName][strtolower(trim($arr[1][$i]))])) 
						$valid = false;
                }
                if($valid) $ByCriteria = $ByNew;
            }
        }
        return $valid;
    }
  
?>
