<?php
    /*
        Описание классов для работы с сервисом Universe-HTT
    */

    class UhttObjSelectCriteria
    {
        var $ObjName;
        var $ByCriteria;
        
        public function __construct($object, $criterion)
        {
            $this->ObjName = $object;
            $this->ByCriteria = $criterion;
        }
    };
    
    class UhttCommObjItem
    {
        var $ID;
        var $ParentID;
        var $Name;
        
        public function __construct($id, $parentid, $name)
        {
            $this->ID = $id;
            $this->ParentID = $parentid;
            $this->Name = $name;
        }
    };

    class UhttWorld
    {
        var $ID;
        var $ParentID;
        var $Name;
        var $Kind;
        var $ParentName;
        var $CountryID;
        var $Latitude;
        var $Longitude;
        var $Code;
        var $CountryIsoAlpha2;
        var $CountryIsoDigit;
        var $CurrencyID;
        
        public function __construct()
        {
            $this->ID = 0;
            $this->ParentID = 0;
            $this->Name = "";
            $this->Kind = 0;
            $this->ParentName = "";
            $this->CountryID = 0;
            $this->Latitude = 0;
            $this->Longitude = 0;
            $this->Code = "";
            $this->CountryIsoAlpha2 = "";
            $this->CountryIsoDigit = 0;
            $this->CurrencyID = 0;
        }
    };
    
    class UhttCurrency
    {
        var $ID;
        var $Name;
        var $Code;
        var $DigitCode;
        
        public function __construct()
        {
            $this->ID = 0;
            $this->Name = "";
            $this->Code = "";
            $this->DigitCode = 0;
        }
    };
    
    class UhttPersonKind
    {
        var $ID;
        var $Name;
        var $Code;
        
        public function __construct()
        {
            $this->ID = 0;
            $this->Name = "";
            $this->Code = "";
        }
    };
    
    class UhttPersonCategory
    {
        var $ID;
        var $Name;
        var $Code;
        
        public function __construct()
        {
            $this->ID = 0;
            $this->Name = "";
            $this->Code = "";
        }
    };
    
    class UhttLocation
    {
        var $LocID;
        var $LocKind;
        var $LocCode;
        var $LocName;
        var $CountryID;
        var $Country;
        var $CityID;
        var $City;
        var $ZIP;
        var $Address;
        var $Latitude;
        var $Longitude;
        
        public function __construct()
        {
            $this->LocID = 0;
            $this->LocKind = 0;
            $this->LocCode = "";
            $this->LocName = "";
            $this->CountryID = 0;
            $this->Country = "";
            $this->CityID = 0;
            $this->City = "";
            $this->ZIP = "";
            $this->Address = "";
            $this->Latitude = 0.0;
            $this->Longitude = 0.0;
        }
    };

    class UhttPersonRegisterType
    {
        var $ID;
        var $Name;
        var $Code;
        
        public function __construct()
        {
            $this->ID = 0;
            $this->Name = "";
            $this->Code = "";
        }
    };

    class UhttPersonRegister
    {
        var $RegID;
        var $RegTypeID;
        var $RegPersonID;
        var $RegOrgID;
        var $RegDt;
        var $RegExpiry;
        var $RegSerial;
        var $RegNumber;
        
        public function __construct($ID, $TypeID, $PersonID, $OrgID, $Dt, $Expiry, $Serial, $Number)
        {
            $this->RegID = $ID;
            $this->RegTypeID = $TypeID;
            $this->RegPersonID = $PersonID;
            $this->RegOrgID = $OrgID;
            $this->RegDt = $Dt;
            $this->RegExpiry = $Expiry;
            $this->RegSerial = $Serial;
            $this->RegNumber = $Number;
        }
    };

    class UhttPerson
    {
        var $ID;
        var $Name;
        var $Code;
        var $KindNameList;
        var $CategoryName;
        var $StatusName;
        var $Addr;
        var $RAddr;
        var $PhoneList;
        var $EMailList;
        var $URL;
        var $RegisterList;
        
        public function __construct()
        {
            $this->ID = 0;
            $this->Name = "";
            $this->Code = "";
            $this->KindNameList = array();
            $this->CategoryName = "";
            $this->StatusName = "";
            $this->Addr = 0;   // object
            $this->RAddr = 0;  // object
            $this->PhoneList = array();
            $this->EMailList = array();
            $this->URL = "";
            $this->RegisterList = array();
        }
    };

    class UhttPersonLocation
    {
        var $PersonID;
        var $Location;
        
        public function __construct()
        {
            $this->PersonID = 0;
            $this->Location = 0; // object
        }
    };

    class UhttGoodsGroup
    {
        var $ID;
        var $ParentID;
        var $Name;
        var $Code;
        
        public function __construct()
        {
            $this->ID = 0;
            $this->ParentID = 0;
            $this->Name = "";
            $this->Code = "";
        }
    };

    class UhttBrand
    {
        var $ID;
        var $Name;
        var $Code;
        var $OwnerID;
        var $GoodsGroupID;
        
        public function __construct()
        {
            $this->ID = 0;
            $this->Name = "";
            $this->Code = "";
            $this->OwnerID = 0;
            $this->GoodsGroupID = 0;
        }
    };

    class UhttBarcode
    {
        var $Code;
        var $Package;
        
        public function __construct($code, $pack)
        {
            $this->Code = $code;
            $this->Package = $pack;
        }
    };

    class UhttArcode
    {
        var $OwnerID;
        var $Code;
        var $Package;
        
        public function __construct($owner, $code, $pack)
        {
            $this->OwnerID = $owner;
            $this->Code = $code;
            $this->Package = $pack;
        }
    };

    class UhttGoodsTaxGroup
    {
        var $ID;
        var $Name;
        var $Code;
        var $VatRate;
        var $ExciseRate;
        var $SalesTaxRate;
        
        public function __construct()
        {
            $this->ID = 0;
            $this->Name = "";
            $this->Code = "";
            $this->VatRate = 0;
            $this->ExciseRate = 0;
            $this->SalesTaxRate = 0;
        }
    };

    class UhttGoods
    {
        var $ID;
        var $ParentID;
        var $Name;
        var $UnitID;
        var $PhUnitID;
        var $PhUPerU;
        var $BrandID;
        var $ManufID;
        var $TaxGrpID;
        var $Package;
        var $BarcodeList;
        
        public function __construct()
        {
            $this->ID = 0;
            $this->ParentID = 0;
            $this->Name = "";
            $this->UnitID = 0;
            $this->PhUnitID = 0;
            $this->PhUPerU = 0.0;
            $this->BrandID = 0;
            $this->ManufID = 0;
            $this->TaxGrpID = 0;
            $this->Package = 0.0;
            $this->BarcodeList = array();
        }
    };

    class UhttCalendar
    {
        var $ID;
        var $ParentID;
        var $Name;
        var $Code;
        var $PersonID;
        var $CountryID;
        
        public function __construct()
        {
            $this->ID = 0;
            $this->ParentID = 0;
            $this->Name = "";
            $this->Code = "";
            $this->PersonID = 0;
            $this->CountryID = 0;
        }
    };

    class UhttCalendarEntry
    {
        var $CalID;
        var $fSkip;
        var $Dt;
        var $TmStart;
        var $TmEnd;
        var $Memo;
        
        public function __construct()
        {
            $this->CalID = 0;
            $this->fSkip = false;
            $this->Dt = Date("Y-m-d");
            $this->TmStart = Date("H:i:s");
            $this->TmStart = Date("H:i:s");
            $this->Memo = "";
        }
    };

    class UhttSpoiledSerial
    {
        var $Serial;
        var $BarCode;
        var $GoodsID;
        var $GoodsName;
        var $ManufID;
        var $ManufName;
        var $ManufCountryID;
        var $ManufCountryName;
        var $LabID;
        var $LabName;
        var $InfoDate;
        var $InfoIdent;
        var $AllowDate;
        var $AllowNumber;
        var $SpecName;
        var $LetterType;
        var $Flags;
        
        public function __construct()
        {
            $this->Serial = "";
            $this->BarCode = "";
            $this->GoodsID = 0;
            $this->GoodsName = "";
            $this->ManufID = 0;
            $this->ManufName = "";
            $this->ManufCountryID = 0;
            $this->ManufCountryName = "";
            $this->LabID = 0;
            $this->LabName = "";
            $this->InfoDate = Date("Y-m-d");
            $this->InfoIdent = "";
            $this->AllowDate = Date("Y-m-d");
            $this->AllowNumber = "";
            $this->SpecName = "";
            $this->LetterType = "";
            $this->Flags = 0;
        }
    };

    class UhttQuot
    {
        var $GoodsID;
        var $SellerID;
        var $BuyerID;
        var $CurID;
        var $Dt;
        var $Tm;
        var $Val;
        var $Memo;
        
        public function __construct()
        {
            $this->GoodsID = 0;
            $this->SellerID = 0;
            $this->BuyerID = 0;
            $this->CurID = 0;
            $this->Dt = Date("Y-m-d");
            $this->Tm = Date("H:i:s");
            $this->Val = 0;
            $this->Memo = "";
        }
    };

    class UhttOutStatus
    {
        var $Status;
        var $Memo;
        
        public function __construct($status, $memo)
        {
            $this->Status = $status;
            $this->Memo = $memo;
        }
    };
?>
