Attribute VB_Name = "slib"
Option Explicit

Private Declare Function OpenProcess Lib "kernel32" _
(ByVal dwDesiredAccess As Long, ByVal bInheritHandle As Long, _
ByVal dwProcessId As Long) As Long

Private Declare Function GetExitCodeProcess Lib "kernel32" _
(ByVal hProcess As Long, lpExitCode As Long) As Long

Private Declare Function FindFirstFile Lib "kernel32" Alias "FindFirstFileA" _
(ByVal lpFileName As String, lpFindFileData As WIN32_FIND_DATA) As Long

Private Declare Function FindNextFile Lib "kernel32" Alias "FindNextFileA" _
(ByVal hFindFile As Long, lpFindFileData As WIN32_FIND_DATA) As Long

Private Declare Function GetFileAttributes Lib "kernel32" Alias "GetFileAttributesA" _
(ByVal lpFileName As String) As Long

Private Declare Function FindClose Lib "kernel32" _
(ByVal hFindFile As Long) As Long

Private Declare Function GetOpenFileName Lib "comdlg32.dll" Alias _
  "GetOpenFileNameA" (pOpenfilename As OPENFILENAME) As Long

Private Const STATUS_PENDING = &H103&
Private Const PROCESS_QUERY_INFORMATION = &H400

Const MAX_PATH = 260
Const MAXDWORD = &HFFFF
Const INVALID_HANDLE_VALUE = -1
Const FILE_ATTRIBUTE_ARCHIVE = &H20
Const FILE_ATTRIBUTE_DIRECTORY = &H10
Const FILE_ATTRIBUTE_HIDDEN = &H2
Const FILE_ATTRIBUTE_NORMAL = &H80
Const FILE_ATTRIBUTE_READONLY = &H1
Const FILE_ATTRIBUTE_SYSTEM = &H4
Const FILE_ATTRIBUTE_TEMPORARY = &H100

Public Const USERERR_CODE = 55
Public Const ERR_EMPTYTAGFOUND = "Найден пустой тэг"

Public Sess As PapyrusInterfaceLib.PPSession
Public Util As PapyrusInterfaceLib.PPUtil

Private PpyLogined As Boolean
Private PpyLoginFail As Boolean

Public Type PpyConfig
    User As String
    password As String
    dbsymb As String
    BhtID  As Long
    ExpPath As String
End Type

Private Type FILETIME
    dwLowDateTime As Long
    dwHighDateTime As Long
End Type

Private Type WIN32_FIND_DATA
    dwFileAttributes As Long
    ftCreationTime As FILETIME
    ftLastAccessTime As FILETIME
    ftLastWriteTime As FILETIME
    nFileSizeHigh As Long
    nFileSizeLow As Long
    dwReserved0 As Long
    dwReserved1 As Long
    cFileName As String * MAX_PATH
    cAlternate As String * 14
End Type

Private Type OPENFILENAME
  lStructSize As Long
  hwndOwner As Long
  hInstance As Long
  lpstrFilter As String
  lpstrCustomFilter As String
  nMaxCustFilter As Long
  nFilterIndex As Long
  lpstrFile As String
  nMaxFile As Long
  lpstrFileTitle As String
  nMaxFileTitle As Long
  lpstrInitialDir As String
  lpstrTitle As String
  flags As Long
  nFileOffset As Integer
  nFileExtension As Integer
  lpstrDefExt As String
  lCustData As Long
  lpfnHook As Long
  lpTemplateName As String
End Type

Public SecurFormOK As Boolean
Public PpyCfg As PpyConfig
'
' if object not nothing return true, else return false
'
Public Function IsValidObj(ByVal o As Variant) As Boolean
    IsValidObj = Not o Is Nothing ' And ObjPtr(o) <> 0
End Function

' Проверяем пустой ли массив
Public Function LongArray_GetCount(ByRef rArray() As Long) As Long
    On Error Resume Next
    Dim l As Long
    l = 0
    l = UBound(rArray) + 1
    LongArray_GetCount = l
End Function

Public Function StringArray_GetCount(ByRef rArray() As String) As Long
    On Error Resume Next
    Dim l As Long
    l = 0
    l = UBound(rArray) + 1
    StringArray_GetCount = l
End Function

Public Function SplitTo_LongArray(ByRef expr As String, ByRef delim As String, ByRef pAry() As Long) As Long
    Dim ok As Long
    ok = 0
    If Len(expr) Then
        Dim strings() As String, count As Long, i As Long, buf As String, v As Long
        strings = Split(expr, delim, -1, vbTextCompare)
        count = StringArray_GetCount(strings)
        If count > 0 Then
            ReDim pAry(count - 1)
            For i = 0 To count - 1
                pAry(i) = CLng(strings(i))
            Next i
            ok = count
        End If
    End If
    SplitTo_LongArray = ok
End Function

Public Function SplitTo_DoubleArray(ByRef expr As String, ByRef delim As String, ByRef pAry() As Double) As Long
    Dim ok As Long
    ok = 0
    If Len(expr) Then
        Dim strings() As String, count As Long, i As Long, buf As String, v As Long
        strings = Split(expr, delim, -1, vbTextCompare)
        count = StringArray_GetCount(strings)
        If count > 0 Then
            ReDim pAry(count - 1)
            For i = 0 To count - 1
                pAry(i) = CDbl(strings(i))
            Next i
            ok = count
        End If
    End If
    SplitTo_DoubleArray = ok
End Function

Public Function SplitTo_StringArray(ByRef expr As String, ByRef delim As String, ByRef pAry() As String) As Long
    Dim ok As Long
    ok = 0
    If Len(expr) Then
        pAry = Split(expr, delim, -1, vbTextCompare)
        ok = StringArray_GetCount(pAry)
    End If
    SplitTo_StringArray = ok
End Function


Public Function SetLastSlash(ByRef pPath As String) As String
    If Len(pPath) > 0 Then
        If Mid(pPath, Len(pPath), 1) <> "\" Then
            pPath = pPath & "\"
        End If
    End If
    SetLastSlash = pPath
End Function

Public Function SplitTo_StrAssocArray(ByRef expr As String, ByRef DELIM As String, ByRef pList As PapyrusInterfaceLib.StrAssocList) As Long
    Dim ok As Long
    ok = 0
    If Len(expr) Then
        Dim strings() As String, count As Long, i As Long, buf As String, v As Long
        strings = Split(expr, DELIM, -1, vbTextCompare)
        count = StringArray_GetCount(strings)
        If count > 0 Then
            For i = 0 To count - 1
                pList.Add CLng(strings(i)), 0, strings(i)
            Next i
            ok = pList.GetCount()
        End If
    End If
    SplitTo_StrAssocArray = ok
End Function

Public Function CalcPctValue(ByVal val1 As Variant, ByVal val2 As Variant, ByVal pct As Integer) As Variant
    Dim ret As Variant
    If val2 Then
        Dim pct_val As Long
        If pct Then
            pct_val = 100
        Else
            pct_val = 1
        End If
        ret = (CDbl(val1) / CDbl(val2)) * pct_val
    Else
        ret = 0
    End If
    CalcPctValue = ret
End Function

Public Sub CellCalcPctValue(ByVal row As Long, ByVal col As Long, ByVal val1 As Variant, ByVal val2 As Variant, ByVal pct As Integer)
    ActiveSheet.Cells(row, col).Value = CalcPctValue(val1, val2, pct)
End Sub

Public Sub ProgressBar(ByRef pView As PapyrusInterfaceLib.IPapyrusView, ByRef pMsg As String, Optional ByVal count As Long, Optional ByVal total As Long)
    Dim counter As PapyrusInterfaceLib.SIterCounter
    Dim msg As String
    msg = pMsg
    If IsValidObj(pView) Then
        counter = pView.GetIterCounter
    Else
        counter.count = count
        counter.total = total
    End If
    If counter.total <> 0 Then
        msg = msg & " " & Round(CDbl(counter.count) / CDbl(counter.total) * 100, 0) & "%"
    End If
    If Len(msg) Then
        Application.StatusBar = msg
    End If
End Sub

Public Function OpenSFile(ByRef path As String, ByVal mode As Integer) As PapyrusInterfaceLib.PPSFile
    Dim file As PapyrusInterfaceLib.PPSFile
    If Len(path) > 0 And mode > 0 Then
        Set file = New PapyrusInterfaceLib.PPSFile
        If IsValidObj(file) Then
            If file.Open(path, mode) <> 1 Then
                Set file = Nothing
                Err.Description = "Ошибка открытия файла: " & path
                Err.Raise 55 ' File already open
            End If
        End If
    End If
    Set OpenSFile = file
End Function

Public Function FileExists(ByVal strFile As String, Optional bFindFolders As Boolean) As Boolean
    'Purpose:   Return True if the file exists, even if it is hidden.
    'Arguments: strFile: File name to look for. Current directory searched if no path included.
    '           bFindFolders. If strFile is a folder, FileExists() returns False unless this argument is True.
    'Note:      Does not look inside subdirectories for the file.
    'Author:    Allen Browne. http://allenbrowne.com June, 2006.
    Dim lngAttributes As Long

    'Include read-only files, hidden files, system files.
    lngAttributes = (vbReadOnly Or vbHidden Or vbSystem)

    If bFindFolders Then
        lngAttributes = (lngAttributes Or vbDirectory) 'Include folders as well.
    Else
        'Strip any trailing slash, so Dir does not look inside the folder.
        Do While Right$(strFile, 1) = "\"
            strFile = Left$(strFile, Len(strFile) - 1)
        Loop
    End If

    'If Dir() returns something, the file exists.
    On Error Resume Next
    FileExists = (Len(dir(strFile, lngAttributes)) > 0)
End Function

Public Function FormatDate(ByVal dt As Date, ByRef delimSymb As String) As String
    Dim d As Integer, m As Integer, y As Integer
    d = Day(dt)
    m = Month(dt)
    y = Year(dt)
    If d > 0 And m > 0 And y > 0 And y > 1900 Then
        FormatDate = d & delimSymb & m & delimSymb & y
    Else
        FormatDate = ""
    End If
End Function

Public Sub CloseSFile(ByRef file As PapyrusInterfaceLib.PPSFile)
    If IsValidObj(file) Then
        file.Close
        Set file = Nothing
    End If
End Sub

Public Sub SetColumnWidth(ByVal ColNum As Integer, ByVal colWidth As Integer, fmt As String)
    Columns(ColNum).Select
    Selection.ColumnWidth = colWidth
    If Len(fmt) Then
        Selection.NumberFormat = fmt
    End If
End Sub

'Public Function Login(Optional ByVal relogin As Boolean, Optional ByVal useLoginFailFlag As Boolean) As Boolean
    'Dim cfg As PpyConfig
    'cfg = LoadPpyConfig
    'Login = LoginByParam(relogin, useLoginFailFlag, cfg.dbsymb, cfg.User, cfg.password)
'End Function

Public Function LoginByParam(ByVal relogin As Boolean, ByVal useLoginFailFlag As Boolean, ByRef dbsymb As String, ByRef UserName As String, ByRef pwd As String) As Boolean
    Dim ok As Boolean, login_fail As Boolean
    ok = False
    login_fail = False
    On Error GoTo errors
    If useLoginFailFlag And PpyLoginFail = True Then
        login_fail = True
    End If
    If login_fail = False Then
        If relogin = True Or PpyLogined = False Then
            Logout
            Set Sess = New PapyrusInterfaceLib.PPSession
            If IsValidObj(Sess) Then
                If Sess.Login(dbsymb, UserName, pwd) = 1 Then
                    PpyLogined = True
                    PpyLoginFail = False
                    Set Util = Sess.CreateSpecClass(spclsUtil)
                    ok = True
                End If
            End If
        Else
            ok = True
        End If
    End If
    GoTo ends
errors:
    PpyLoginFail = True
    MsgBox (Err.Description)
    Err.Clear
    ok = False
ends:
    LoginByParam = ok
End Function

Public Sub SetLoggedInFlag()
    PpyLogined = True
End Sub

Public Function IsLogined() As Boolean
    IsLogined = PpyLogined
End Function

Public Sub Logout()
    If PpyLogined Then
        Sess.Logout
    End If
    Set Sess = Nothing
    Set Util = Nothing
    PpyLogined = False
End Sub
'
' Descr: ‘®§¤ Ґв а Ў®зЁ© «Ёбв Excel б Ё¬Ґ­Ґ¬ name.
'   …б«Ё «Ёбв б в ЄЁ¬ Ё¬Ґ­Ґ¬ г¦Ґ бгйҐбвўгҐв, в® ­ЁзҐЈ® ­Ґ ¤Ґ« Ґв
'
Sub CreateSheet(ByRef name As String)
    Dim is_sheet_exists As Integer
    Dim i As Integer
    Dim c As Integer
    c = ActiveWorkbook.Sheets.count
    While i < c And is_sheet_exists = 0
        If ActiveWorkbook.Sheets.item(i + 1).name = name Then
            is_sheet_exists = 1
        End If
        i = i + 1
    Wend
    If is_sheet_exists = 0 Then
        ActiveWorkbook.Worksheets.Add.name = name
    End If
End Sub

Public Function FindString(ByVal begRow As Long, ByVal begCol As Long, ByVal endRow As Long, ByVal endCol As Long, _
    ByRef pSrchStr As String, ByVal matchCase As Boolean, ByRef pFindRow As Long, ByRef pFindCol As Long) As Boolean
    
    Dim ok As Boolean
    ok = False
    If Len(pSrchStr) > 0 Then
        Dim found_cell As Range, last_cell As Range, search_range As Range
        Dim str_srch_range As String
        
        Set search_range = ActiveSheet.Range(ActiveSheet.Cells(begRow, begCol), ActiveSheet.Cells(endRow, endCol))
        Set last_cell = search_range.Cells(search_range.Cells.count)
        Set found_cell = search_range.Find(what:=pSrchStr, after:=last_cell, _
            LookIn:=xlValues, LookAt:=xlWhole, SearchOrder:=xlByRows, matchCase:=matchCase)
        If Not found_cell Is Nothing Then
            ok = True
            pFindRow = found_cell.row
            pFindCol = found_cell.Column
        End If
    End If
    FindString = ok
End Function
'
' Работа с xml данными
'
Public Sub WriteXmlHeader(ByRef pDir As String, ByRef pName As String, Optional ByVal cvtTo1251 As Boolean = True)
    Dim i As Long
    Dim temp_buf As String, path As String, temp_path As String
    Dim temp_file As PapyrusInterfaceLib.PPSFile, file As PapyrusInterfaceLib.PPSFile
    
    i = 0
    temp_path = Util.ToChar(pDir & pName & ".xml_")
    Set temp_file = OpenSFile(temp_path, mRead)
    path = Util.ToChar(pDir & pName & ".xml")
    Set file = OpenSFile(path, mWrite)
    While temp_file.ReadLine(temp_buf) > 0
        If cvtTo1251 Then
            If i = 0 Then
                temp_buf = "<?xml version=""1.0"" encoding=""windows-1251""?>"
            Else
                temp_buf = Util.UTF8ToChar(temp_buf)
            End If
        Else
            If i = 0 Then
                temp_buf = "<?xml version=""1.0"" encoding=""UTF-8""?>"
            End If
        End If
        file.WriteLine temp_buf & vbLf
        i = i + 1
    Wend
    CloseSFile file
    CloseSFile temp_file
    Util.RemoveFile temp_path
End Sub

Sub InitDocument(ByRef xmlDoc As MSXML2.DOMDocument30, ByRef nodeStruct As String)
    Dim pi As IXMLDOMProcessingInstruction
    xmlDoc.async = False
    xmlDoc.loadXML nodeStruct
    Set pi = xmlDoc.createProcessingInstruction("xml", "version='1.0'")
    xmlDoc.insertBefore pi, xmlDoc.childNodes.item(0)
    Set pi = Nothing
End Sub

Sub WriteLastElemAttribute(ByRef xmlDoc As MSXML2.DOMDocument30, ByRef xmlNode As MSXML2.IXMLDOMNode, ByRef attribName As String, ByRef attribVal As String, Optional ByVal nodeAttrib As Boolean = False)
    Dim new_attrib As MSXML2.IXMLDOMAttribute
    Set new_attrib = xmlDoc.createAttribute(attribName)
    new_attrib.Text = attribVal
    If nodeAttrib Then
        xmlNode.Attributes.setNamedItem new_attrib
    Else
        xmlNode.LastChild.Attributes.setNamedItem new_attrib
    End If
    Set new_attrib = Nothing
End Sub

Sub WriteLastElemBoolAttribute(ByRef xmlDoc As MSXML2.DOMDocument30, ByRef xmlNode As MSXML2.IXMLDOMNode, ByRef attribName As String, ByRef attribVal As Boolean, Optional ByVal nodeAttrib As Boolean = False)
    Dim new_attrib As MSXML2.IXMLDOMAttribute
    Set new_attrib = xmlDoc.createAttribute(attribName)
    new_attrib.DataType = "boolean"
    new_attrib.nodeTypedValue = attribVal
    ' new_attrib.Text = attribVal
    If nodeAttrib Then
        xmlNode.Attributes.setNamedItem new_attrib
    Else
        xmlNode.LastChild.Attributes.setNamedItem new_attrib
    End If
    Set new_attrib = Nothing
End Sub

Sub WriteElement(ByRef xmlDoc As MSXML2.DOMDocument30, ByRef xmlNode As MSXML2.IXMLDOMNode, ByRef elemName As String, ByRef elemValue As String)
    If IsValidObj(xmlDoc) And IsValidObj(xmlNode) Then
        Dim new_elem As IXMLDOMElement
        Set new_elem = xmlDoc.createElement(elemName)
        xmlNode.appendChild new_elem
        xmlNode.LastChild.Text = elemValue
        Set new_elem = Nothing
    End If
End Sub

Function GetAttribValue(ByRef xmlNode As MSXML2.IXMLDOMNode, ByRef tagName As String, ByRef tagValue As String) As Boolean
    Dim attr_node As MSXML2.IXMLDOMNode
    Set attr_node = xmlNode.Attributes.getNamedItem(tagName)
    tagValue = ""
    If IsValidObj(attr_node) Then
        If IsValidObj(attr_node.LastChild) Then
            tagValue = attr_node.LastChild.Text
        End If
    End If
    Set attr_node = Nothing
End Function

Function GetTagValue(ByRef xmlNode As MSXML2.IXMLDOMNode, ByRef tagName As String, ByRef tagValue As String) As Boolean
    Dim ok As Boolean
    Dim elem As MSXML2.IXMLDOMNode
    
    ok = False
    tagValue = ""
    Set elem = xmlNode.selectSingleNode(tagName)
    If IsValidObj(elem) Then
        If IsValidObj(elem.LastChild) Then
            tagValue = elem.LastChild.Text
            ok = True
        Else
            Err.Description = "Элемент=" & tagName & " не определен"
            Err.Raise USERERR_CODE
        End If
    Else
        Err.Description = "Элемент=" & tagName & " не определен"
        Err.Raise USERERR_CODE
    End If
    GetTagValue = ok
End Function

Sub InitNodes(ByRef xmlDoc As MSXML2.DOMDocument30, ByRef rootNode As MSXML2.IXMLDOMNode, ByRef node As MSXML2.IXMLDOMNode, ByVal rootName As String, ByVal nodeName As String, ByRef pRootPrefx As String)
    If IsValidObj(rootNode) = False Then
        Dim root_name As String
        If Len(pRootPrefx) Then
            root_name = "//" & pRootPrefx
        End If
        If Len(rootName) Then
            root_name = root_name & "/" & rootName
        End If
        Set rootNode = xmlDoc.selectSingleNode(root_name)
    End If
    Set node = Nothing
    If Len(nodeName) > 0 Then
        Set node = xmlDoc.createNode(1, nodeName, "")
    End If
End Sub

Function MakeNodeStruct(ByVal rootElem As String, ByRef rBuf As String) As String
    If Len(rootElem) Then
        rBuf = rBuf & "<" & rootElem & ">" & vbLf & "</" & rootElem & ">" & vbLf
    Else
        Err.Description = ERR_EMPTYTAGFOUND
        Err.Raise USERERR_CODE
    End If
    MakeNodeStruct = rBuf
End Function

Public Sub AppendChildAndDestroy(ByRef pRootNode As MSXML2.IXMLDOMNode, ByRef pChildNode As MSXML2.IXMLDOMNode)
    If IsValidObj(pRootNode) And IsValidObj(pChildNode) Then
        pRootNode.appendChild pChildNode
        ' pRootNode.insertBefore pChildNode, pRootNode.FirstChild
    End If
    Set pChildNode = Nothing
End Sub
'
'
'
Function IsValidGoodsID(ByVal goodsID As Long, ByVal AltGrpID As Long, ByVal SupplID, ByRef pAdvGObj As PapyrusInterfaceLib.IPapyrusObjGoods, ByRef pArCode As String, Optional ByVal barcodeAsArcode As Boolean = False) As Boolean
    Dim valid As Boolean
    valid = True
    pArCode = ""
    If AltGrpID And IsValidObj(pAdvGObj) Then
        If pAdvGObj.BelongToGroup(goodsID, AltGrpID, 0) <= 0 Then
            valid = False
        End If
    End If
    If valid = True Then
        If barcodeAsArcode Then
            pArCode = pAdvGObj.GetArCode(goodsID, SupplID)
        Else
            Dim codes As PapyrusInterfaceLib.StrAssocList
            Set codes = pAdvGObj.GetBarcodes(goodsID)
            If codes.GetCount > 0 Then
                Dim item As PapyrusInterfaceLib.STaggedString
                If codes.Get(0, item) > 0 Then
                    pArCode = item.Text
                End If
            End If
            Set codes = Nothing
        End If
        If Len(pArCode) <= 0 Then
            valid = False
        End If
    End If
    IsValidGoodsID = valid
End Function

Function GetGoodsIDByArCode(ByRef pCode, ByVal SupplID, ByRef pAdvGObj As PapyrusInterfaceLib.IPapyrusObjGoods, ByRef pGoodsID As Long) As Boolean
    Dim valid As Boolean
    valid = True
    pGoodsID = False
    If IsValidObj(pAdvGObj) Then
        Dim grec As PapyrusInterfaceLib.SPpyO_Goods
        If pAdvGObj.SearchByArCode(SupplID, pCode, grec) > 0 Then
            pGoodsID = grec.id
            valid = True
        End If
    End If
    GetGoodsIDByArCode = valid
End Function

Sub outMessage(ByRef msg As String, ByRef pLogFile As PapyrusInterfaceLib.PPSFile, Optional ByVal useAutoMode = 0, Optional ByVal rowNumber = 0)
    If useAutoMode Then
        If IsValidObj(pLogFile) Then
            If pLogFile.IsValid() Then
                pLogFile.WriteLine Date & " " & Time & "   " & msg & vbLf
            End If
        End If
    Else
        If rowNumber > 0 Then
            ActiveSheet.Cells(rowNumber, 1).Value = msg
        Else
            MsgBox msg
        End If
    End If
End Sub

Public Function AddDbfFld(ByRef pFlds As PapyrusInterfaceLib.PPDbfCreateFlds, ByRef pName As String, ByVal fldType As Integer, ByVal size As Long, ByVal prec As Long) As Boolean
    Dim ok As Boolean
    Dim fld As PapyrusInterfaceLib.SDbfCreateFld
    ok = False
    If IsValidObj(pFlds) Then
        fld.name = pName
        fld.Type = fldType
        fld.size = size
        fld.prec = prec
        If pFlds.Add(fld) > 0 Then
            ok = True
        End If
    End If
    AddDbfFld = ok
End Function

Public Function GetOrderByBill(ByRef pBPack As PapyrusInterfaceLib.PPBillPacket, ByRef pBObj As PapyrusInterfaceLib.PPObjBill, ByRef pOrdRec As PapyrusInterfaceLib.SPpyO_Bill) As Long
    Dim ok As Long
    Dim orders_list As PapyrusInterfaceLib.StrAssocList
    ok = -1
    Set orders_list = pBPack.GetOrderList
    If IsValidObj(orders_list) Then
        If orders_list.GetCount() > 0 Then
            Dim ord_item As PapyrusInterfaceLib.STaggedString
            orders_list.Get 0, ord_item
            If pBObj.Search(ord_item.id, pOrdRec) > 0 Then
                ok = 1
            End If
        End If
    End If
    Set orders_list = Nothing
    GetOrderByBill = ok
End Function

Public Sub XmlReplaceSpecSymbs(ByRef pBuf As String)
    pBuf = Replace(pBuf, "&", "&amp;", 1, -1, vbTextCompare)
    pBuf = Replace(pBuf, """", "&quot;", 1, -1, vbTextCompare)
    pBuf = Replace(pBuf, "<", "&gt;", 1, -1, vbTextCompare)
    pBuf = Replace(pBuf, ">", "&lt;", 1, -1, vbTextCompare)
    pBuf = Replace(pBuf, "'", "&apos;", 1, -1, vbTextCompare)
End Sub

Public Function ShellandWait(ExeFullPath As String, _
Optional TimeOutValue As Long = 0) As Boolean
    
    Dim lInst As Long
    Dim lStart As Long
    Dim lTimeToQuit As Long
    Dim sExeName As String
    Dim lProcessId As Long
    Dim lExitCode As Long
    Dim bPastMidnight As Boolean
    
    On Error GoTo ErrorHandler

    lStart = CLng(Timer)
    sExeName = ExeFullPath

    'Deal with timeout being reset at Midnight
    If TimeOutValue > 0 Then
        If lStart + TimeOutValue < 86400 Then
            lTimeToQuit = lStart + TimeOutValue
        Else
            lTimeToQuit = (lStart - 86400) + TimeOutValue
            bPastMidnight = True
        End If
    End If

    lInst = Shell(sExeName, vbMinimizedNoFocus)
    lProcessId = OpenProcess(PROCESS_QUERY_INFORMATION, False, lInst)

    Do
        Call GetExitCodeProcess(lProcessId, lExitCode)
        DoEvents
        If TimeOutValue And Timer > lTimeToQuit Then
            If bPastMidnight Then
                 If Timer < lStart Then Exit Do
            Else
                 Exit Do
            End If
    End If
    Loop While lExitCode = STATUS_PENDING
    
    ShellandWait = True
   
ErrorHandler:
ShellandWait = False
Exit Function
End Function

' Ищет документ по id и возвращает id агента или 0 если не установлен
Public Function GetAgentByBillID(ByVal billID As Long, ByRef pBObj As PapyrusInterfaceLib.PPObjBill) As Long
    Dim agent_id As Long
    agent_id = 0
    If billID > 0 Then
        Dim bill_rec As PapyrusInterfaceLib.SPpyO_Bill
        If pBObj.Search(billID, bill_rec) > 0 Then
            agent_id = bill_rec.AgentID
        End If
    End If
    GetAgentByBillID = agent_id
End Function

Function StripNulls(OriginalStr As String) As String
    If (InStr(OriginalStr, Chr(0)) > 0) Then
        OriginalStr = Left(OriginalStr, InStr(OriginalStr, Chr(0)) - 1)
    End If
    StripNulls = OriginalStr
End Function

Public Function GetFileList(pPath As String, wildcard As String, ByRef pFilesList() As String) As Long
    Dim fname As String, path As String
    Dim handle As Long ' Search Handle
    Dim WFD As WIN32_FIND_DATA
    Dim cont As Integer
        
    ReDim pFilesList(0)
    path = SetLastSlash(pPath)

    handle = FindFirstFile(path & wildcard, WFD)
    cont = 1
    If handle <> INVALID_HANDLE_VALUE Then
        While cont
            fname = StripNulls(WFD.cFileName)
            If (fname <> ".") And (fname <> "..") Then
                Dim idx As Long
                
                idx = UBound(pFilesList)
                pFilesList(idx) = path & fname
                ReDim Preserve pFilesList(idx + 1)
            End If
            cont = FindNextFile(handle, WFD) ' Get next file
        Wend
        cont = FindClose(handle)
    End If
    If StringArray_GetCount(pFilesList) > 0 Then
        GetFileList = 1
    Else
        GetFileList = -1
    End If
End Function

 'The following function returns the filename with the extension from the file's full path:
Function FileNameWithExt(strPath As String) As String
    FileNameWithExt = Mid$(strPath, InStrRev(strPath, "\") + 1)
End Function

Function FileNameWoExt(strPath As String) As String
    Dim fname As String, strs() As String
    
    fname = Mid$(strPath, InStrRev(strPath, "\") + 1)
    strs = Split(fname, ".", , vbTextCompare)
    If UBound(strs) > 0 Then
        fname = strs(0)
    End If
    FileNameWoExt = fname
End Function

Public Function FindExcelFile(ByRef filePath As String, ByRef cancelled As Boolean) As Long
    Dim r As Long
    Dim OpenFile As OPENFILENAME
    Dim lReturn As Long
    Dim sFilter As String
    
    On Error GoTo errors
    
    OpenFile.lStructSize = Len(OpenFile)

    '// Sample filter:
    '// "Text Files (*.txt)" & Chr$(0) & "*.sky" & Chr$(0) & "All Files (*.*)" & Chr$(0) & "*.*"
    sFilter = "Excel Files (*.xl*)" & Chr(0) & "*.xl*"
    
    OpenFile.lpstrFilter = sFilter
    OpenFile.nFilterIndex = 1
    OpenFile.lpstrFile = String(257, 0)
    OpenFile.nMaxFile = Len(OpenFile.lpstrFile) - 1
    OpenFile.lpstrFileTitle = OpenFile.lpstrFile
    OpenFile.nMaxFileTitle = OpenFile.nMaxFile
    OpenFile.lpstrInitialDir = ThisDocument.path
    
    OpenFile.lpstrTitle = "Find Excel Data Source"
    OpenFile.flags = 0
    lReturn = GetOpenFileName(OpenFile)
    
    If lReturn = 0 Then
       r = -1
       filePath = vbNullString
    Else
        r = 1
        filePath = Trim(OpenFile.lpstrFile)
        filePath = Replace(filePath, Chr(0), vbNullString)
    End If
    GoTo ends
errors:
    r = 0
    MsgBox Err.Description
    Err.Clear
ends:
    FindExcelFile = r
End Function

Public Function DoubleToStr(ByVal val As Double) As String
    DoubleToStr = Format(val, "0.00")
End Function

Public Function CalcWeekEndDt(ByVal startDt As Date) As Date
    Dim start_mon As Long, end_mon As Long, end_dt As Date
    start_mon = Month(startDt)
    end_dt = DateAdd("d", 7 - Weekday(startDt, vbMonday), startDt)
    end_mon = Month(end_dt)
    If start_mon <> end_mon Then
        end_dt = CDate("01." & Month(end_dt) & "." & Year(end_dt))
        end_dt = DateAdd("d", -1, end_dt)
    End If
    CalcEndWeekDt = end_dt
End Function

Private Function GetDlvrAddress(ByVal dlvrID As Long, ByRef pLocObj as PapyrusInterfaceLib.PPObjLocation) As String
    Dim dlvr_addr As String
    Dim loc_rec As PapyrusInterfaceLib.SPpyO_Location
    If LocObj.Search(dlvrID, loc_rec) > 0 Then
        dlvr_addr = AdvLocObj.GetAddress(dlvrID)
    End If
    GetDlvrAddress = dlvr_addr
End Function
