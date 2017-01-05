Option Strict Off
Imports PPY = PapyrusInterfaceLib

Module Test_IfcClient
    Private Const dbsymb As String = "land_store1"
    Private Const username As String = "master"
    Private Const userpassword As String = "123"

    Public ppsess As PPY.PPSession

    Sub Login()
        If (ppsess Is Nothing) Then
            ppsess = New PPY.PPSession
        End If
        Call ppsess.Login(dbsymb, username, userpassword)
    End Sub

    Sub Logout()
        If (Not ppsess Is Nothing) Then
            Call ppsess.Logout()
            ppsess = Nothing
        End If
    End Sub
    '
    ' Тестирует функции PPSession.GetDatabaseList и PPSession.GetDatabaseInfo
    '
    Sub TestDbEntryList()
        On Error GoTo on_error
        If (ppsess Is Nothing) Then
            ppsess = New PPY.PPSession
        End If
        Dim db_list As PPY.IStrAssocList
        Dim db_list_item As PPY.STaggedString
        db_list = ppsess.GetDatabaseList(0)
        While db_list.NextIteration(db_list_item)
            Dim db_info As PPY.SPpyDatabaseInfo
            Call ppsess.GetDatabaseInfo(db_list_item.id, db_info)
        End While
        GoTo epilog
on_error:
        MsgBox(Err.Description)
epilog:
        db_list = Nothing
    End Sub
    '
    ' Тестирует функцию PPSession.GetStatusInfo
    '
    Sub TestGetStatusInfo()
        On Error GoTo on_error
        Call Login()
        Dim status_info As PPY.SPpySessionInfo
        Call ppsess.GetStatusInfo(status_info)
        GoTo epilog
on_error:
        MsgBox(Err.Description)
epilog:
        Call Logout()
    End Sub
    '
    ' Тестирует авторизацию в базе данных.
    ' Многократно авторизуется и выходит из базы данных.
    '
    Sub TestLogin()
        On Error GoTo on_error
        Dim i As Integer
        For i = 0 To 20
            Call Login()
            Call Logout()
        Next i
        GoTo epilog
on_error:
        MsgBox(Err.Description)
epilog:
        Call Logout()
    End Sub
    '
    ' Тест занесения в базу данных списка статей аналитического учета по таблице "Поставщики"
    '
    Sub TestList()
        On Error GoTo on_error
        Call Login()
        Dim acs_obj As PPY.IPapyrusObject ' PPObjAccSheet
        Dim acs_rec As PPY.SPpyO_AccSheet ' Запись таблицы аналитических статей
        Dim ar_obj As PPY.IPapyrusObject
        Dim ar_rec As PPY.SPpyO_Article
        acs_obj = ppsess.CreateObject(PPY._PpyObjectIdent.ppoAccSheet)
        '
        ' Ищем таблицу статей с именем "Поставщики"
        '
        If acs_obj.SearchByName("Поставщики", 0, 0, acs_rec) > 0 Then
            ar_obj = ppsess.CreateObject(PPY._PpyObjectIdent.ppoArticle)
            Dim ar_list As IStrAssocList
            '
            ' Создаем список статей, соответствующих таблице "Поставщики"
            ' (идентификатор этой таблицы =acs_rec.id
            '
            ar_list = ar_obj.GetSelector(acs_rec.id)
            Dim ar_item As PPY.STaggedString
            '
            ' Тест вставки элемента в список строк
            '
            ar_list.Add(777, 0, "ABC 001")
            '
            ' Вызов ar_list InitIteration сразу после создания //
            ' списка избыточен, поскольку при создании внутренний
            ' счетчик списка и так установлен в 0.
            '
            ar_list.InitIteration()
            While ar_list.NextIteration(ar_item) > 0
                '
                ' Создаем SQL-запрос вида "insert into ObjectList (id, objtype, name) values (id, objtype, name)
                '
                Dim stmt As String
                stmt = "insert into ObjectList ( id, objtype, name ) values (" + CStr(ar_item.id) + "," _
                    + "1006" + "," + "'" + ar_item.text + "'" + ")"
                '
                ' Выполняем SQL-запрос
                '
                'DoCmd.RunSQL(stmt)
            End While
        End If
        GoTo epilog
on_error:
        MsgBox(Err.Description)
epilog:
        ar_list = Nothing
        ar_obj = Nothing
        acs_obj = Nothing
        '
        ' Если забыть выполнить Logout, то через пару сеансов при попытке
        ' вызвать Login появится сообщение "Ошибка Btrieve 3006"
        '
        Call Logout()
    End Sub

    Sub Main()
        On Error GoTo catch_point
        Console.WriteLine("This is Papyrus COM-object's testing")
        'Dim sess As New Ppy.PPSession

        'Dim ora As OracleInProcServer.OraServerClassClass
        'ora = New OracleInProcServer.OraServerClassClass

        Dim util As PPY.PPUtil
        util = New PPY.PPUtil

        Dim sess As PPY.PPSession
        sess = New PPY.PPSession
        sess.Login("land_store1", "master", "123")
        sess.Logout()
        GoTo epilog
catch_point:
        Console.WriteLine(Err.Description)
epilog:
        sess = Nothing
    End Sub
End Module