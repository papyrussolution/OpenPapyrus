// Database.java
// Copyright (c) A.Sobolev 2021
//
package ru.petroglif.styloq;

import android.content.ContentValues;
import android.content.Context;
import android.database.SQLException;
import android.database.sqlite.SQLiteCursor;
import android.database.sqlite.SQLiteCursorDriver;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQuery;
import android.os.Bundle;
import android.util.Log;
import org.jetbrains.annotations.NotNull;
import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.Base64;

public class Database {
	protected final Context Ctx;
	private String Name;
	protected SQLiteDatabase DB;
	protected Class<?> TableClasses[];
	protected String TableNames[]; // Для ускорения функции IsTableExists
	private DatabaseHelper DBHelper;
	//
	private static class DatabaseHelper extends SQLiteOpenHelper {
		private Database DB;
		DatabaseHelper(Context context, Database db, int dbVer)
		{
			super(context, db.Name, null, dbVer);
			DB = db;
		}
		@Override
		public void onCreate(SQLiteDatabase db) throws SQLiteException
		{
			try {
				DB.onCreate(db);
			} catch(StyloQException exn) {
				new SQLiteException(exn.GetMessage(DB.Ctx));
			}
		}
		@Override
		public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) throws SQLiteException
		{
			try {
				DB.onUpgrade(db, oldVersion, newVersion);
			} catch(StyloQException exn) {
				new SQLiteException(exn.GetMessage(DB.Ctx));
			}
		}
	}
	static public class Table {
		private String TableName;
		protected Context Ctx;

		public Table()
		{
		}
		public Table(Context context, String tableName)
		{
			TableName = tableName;
			Ctx = context;
		}
		public void Init(Context context, String tableName)
		{
			TableName = tableName;
			Ctx = context;
		}
		public String GetName() { return TableName; }
		public SQLiteDatabase GetDB() {
			return StyloQApp.GetDBHandle(Ctx);
		}
		public long Insert(ContentValues pRec) throws StyloQException
		{
			long dbpos = 0;
			try {
				SQLiteDatabase db = GetDB();
				dbpos = db.insertOrThrow(TableName, null, pRec);
			}
			catch(SQLException exn) {
				new StyloQException(ppstr2.PPERR_JEXN_SQL, exn.getMessage());
			}
			return  dbpos;
		}
		public long Insert(Record rec) throws StyloQException
		{
			ContentValues data = new ContentValues();
			data = rec.Get();
			return Insert(data);
		}
		public int UpdateByID(long id, Record rec)
		{
			SQLiteDatabase db = GetDB();
			ContentValues data = rec.Get();
			return db.update(TableName, data, "ID=" + id, null);
		}
		public int RemoveByID(long id)
		{
			int ok = -1;
			SQLiteDatabase db = GetDB();
			android.database.Cursor cursor = db.rawQuery("DELETE FROM " + TableName + " WHERE ID=" + id, null);
			if(cursor != null) {
				cursor.moveToFirst();
				cursor.close();
				ok = 1;
			}
			return ok;
		}
	}

	static public class Field {
		public String Name;
		public int Type;
		public String SqlType;
		public java.lang.reflect.Field Rf;
	}

	static public class Record {
		public Record() throws StyloQException
		{
			FieldList = new ArrayList<Field>();
			java.lang.reflect.Field[] flds = getClass().getFields();
			final int count = flds.length;
			for(int i = 0; i < count; i++) {
				java.lang.reflect.Field fld = flds[i];
				Field fld_info = new Field();
				fld_info.Name = fld.getName();
				fld_info.SqlType = "";
				fld_info.Rf = fld;
				if(fld.getType().isArray()) {
					fld_info.SqlType = "BLOB";
					fld_info.Type = DataType.S_RAW;
				}
				else {
					String fld_type = fld.getType().getSimpleName();
					if(fld_type.equalsIgnoreCase("int")) {
						fld_info.SqlType = "INTEGER";
						fld_info.Type = DataType.T_INT32();
					}
					else if(fld_type.equalsIgnoreCase("long")) {
						fld_info.SqlType = "LONG";
						fld_info.Type = DataType.T_INT64();
					}
					else if(fld_type.equalsIgnoreCase("String")) {
						fld_info.SqlType = "TEXT";
						fld_info.Type = DataType.S_ZSTRING;
					}
					else if(fld_type.equalsIgnoreCase("double")) {
						fld_info.SqlType = "DOUBLE";
						fld_info.Type = DataType.T_DOUBLE();
					}
				}
				FieldList.add(fld_info);
			}
			Init();
		}
		public void Init() throws StyloQException
		{
			final int count = FieldList.size();
			for(int i = 0; i < count; i++) {
				Field fld_info = FieldList.get(i);
				java.lang.reflect.Field fld = fld_info.Rf;//Cls.getDeclaredField(fld_info.Name);
				//SetValue(fld, fld_info.Type, "");
				ClearValue(fld, fld_info.Type);
			}
		}
		public java.lang.reflect.Field GetFld(String fldName)
		{
			java.lang.reflect.Field rf = null;
			final int count = FieldList.size();
			for(int i = 0; rf == null && i < count; i++) {
				Field fld_info = FieldList.get(i);
				if(fld_info.Name.equalsIgnoreCase(fldName))
					rf = fld_info.Rf;
			}
			return rf;
		}
		public int SetValue(String fldName, int val)
		{
			return SetValue(fldName, String.valueOf(val));
		}
		public int SetValue(String fldName, String val)
		{
			int ok = 1;
			try {
				java.lang.reflect.Field fld = GetFld(fldName);// Cls.getDeclaredField(fldName);
				if(fld != null) {
					if(fld.getType().isArray()) {
						fld.set(this, Base64.getDecoder().decode(val));
					}
					else {
						String fld_type = fld.getType().getSimpleName();
						if(fld_type.equalsIgnoreCase("int")) {
							fld.setInt(this, SLib.satoi(val));
						}
						else if(fld_type.equalsIgnoreCase("long")) {
							fld.setLong(this, Long.valueOf(val));
						}
						else if(fld_type.equalsIgnoreCase("String")) {
							fld.set(this, val);
						}
						else if(fld_type.equalsIgnoreCase("double")) {
							fld.set(this, Double.valueOf(val));
						}
					}
				}
				else
					ok = -1;
			} catch(Exception e) {
				e.printStackTrace();
				ok = 0;
			}
			return ok;
		}
		private void ClearValue(java.lang.reflect.Field fld, int fldType) throws StyloQException
		{
			try {
				if(fldType == DataType.T_INT32()) {
					fld.setInt(this, 0);
				}
				else if(fldType == DataType.T_INT64()) {
					fld.setLong(this, 0);
				}
				else if(fldType == DataType.S_ZSTRING) {
					fld.set(this, "");
				}
				else if(fldType == DataType.T_DOUBLE()) {
					fld.setDouble(this, 0.0);
				}
				else if(fldType == DataType.S_RAW) {
					fld.set(this, null);
				}
			} catch(IllegalAccessException exn) {
				new StyloQException(ppstr2.PPERR_JEXN_ILLEGALACCESS, exn.getMessage());
			}
		}
		private void SetValue(java.lang.reflect.Field fld, int fldType, String val) throws StyloQException
		{
			try {
				if(fldType == DataType.T_INT32()) {
					fld.setInt(this, Integer.parseInt(val));
				}
				else if(fldType == DataType.T_INT64()) {
					fld.setLong(this, Long.parseLong(val));
				}
				else if(fldType == DataType.S_ZSTRING) {
					fld.set(this, val);
				}
				else if(fldType == DataType.T_DOUBLE()) {
					fld.setDouble(this, SLib.strtodouble(val));
				}
				else if(fldType == DataType.S_RAW) {
					fld.set(this, Base64.getDecoder().decode(val));
				}
			} catch(IllegalAccessException exn) {
				new StyloQException(ppstr2.PPERR_JEXN_ILLEGALACCESS, exn.getMessage());
			}
		}
		public Bundle GetBundle(Bundle data)
		{
			if(data == null)
				data = new Bundle();
			final int count = FieldList.size();
			try {
				for(int i = 0; i < count; i++) {
					Field fld_info = FieldList.get(i);
					java.lang.reflect.Field fld = fld_info.Rf; //Cls.getDeclaredField(fld_info.Name);
					if(fld_info.Type == DataType.T_INT32())
						data.putInt(fld_info.Name, fld.getInt(this));
					else if(fld_info.Type ==DataType.T_INT64())
						data.putLong(fld_info.Name, fld.getLong(this));
					else if(fld_info.Type == DataType.S_ZSTRING)
						data.putString(fld_info.Name, (String)fld.get(this));
					else if(fld_info.Type == DataType.T_DOUBLE())
						data.putDouble(fld_info.Name, fld.getDouble(this));
					else if(fld_info.Type == DataType.S_RAW) {
						Object _obj = fld.get(this);
						final int bl = Array.getLength(_obj);
						if(bl > 0) {
							byte [] temp_bytes = new byte[bl];
							for(int bc = 0; bc < bl; bc++) {
								temp_bytes[bc] = Array.getByte(_obj, bc);
							}
							data.putByteArray(fld_info.Name, temp_bytes);
						}
					}
				}
			}
			catch(Exception e) {
				e.printStackTrace();
			}
			return data;
		}
		public Bundle GetBundle()
		{
			return GetBundle(null);
		}
		public boolean Set(ContentValues data) throws StyloQException
		{
			boolean ok = true;
			Init();
			if(data != null) {
				final int count = FieldList.size();
				try {
					for(int i = 0; i < count; i++) {
						Field fld_info = FieldList.get(i);
						if(fld_info != null && data.get(fld_info.Name) != null) {
							java.lang.reflect.Field fld = fld_info.Rf; //Cls.getDeclaredField(fld_info.Name);
							//SetValue(fld, fld_info.Type, data.getAsString(fld_info.Name));
							if(fld_info.Type == DataType.T_INT32())
								fld.setInt(this, data.getAsInteger(fld_info.Name));
							else if(fld_info.Type == DataType.T_INT64()) {
								fld.setLong(this, data.getAsLong(fld_info.Name));
							}
							else if(fld_info.Type == DataType.S_ZSTRING)
								fld.set(this, data.getAsString(fld_info.Name));
							else if(fld_info.Type == DataType.T_DOUBLE())
								fld.setDouble(this, data.getAsDouble(fld_info.Name));
							else if(fld_info.Type == DataType.S_RAW)
								fld.set(this, data.getAsByteArray(fld_info.Name));
						}
					}
				} catch(IllegalAccessException exn) {
					ok = false;
					new StyloQException(ppstr2.PPERR_JEXN_ILLEGALACCESS, exn.getMessage());
				}
			}
			else {
				Init();
			}
			return ok;
		}
		public boolean Set(Bundle data) throws StyloQException
		{
			boolean ok = true;
			if(data != null) {
				final int count = FieldList.size();
				try {
					for(int i = 0; i < count; i++) {
						Field fld_info = FieldList.get(i);
						java.lang.reflect.Field fld = fld_info.Rf;//Cls.getDeclaredField(fld_info.Name);
						if(fld_info.Type == DataType.T_INT32())
							fld.setInt(this, data.getInt(fld_info.Name, 0));
						else if(fld_info.Type == DataType.T_INT64())
							fld.setLong(this, data.getLong(fld_info.Name, 0));
						else if(fld_info.Type == DataType.S_ZSTRING)
							fld.set(this, data.getString(fld_info.Name));
						else if(fld_info.Type == DataType.T_DOUBLE())
							fld.setDouble(this, data.getDouble(fld_info.Name, 0.0));
						else if(fld_info.Type == DataType.S_RAW)
							fld.set(this, data.getByteArray(fld_info.Name));
					}
				} catch(IllegalAccessException exn) {
					ok = false;
					new StyloQException(ppstr2.PPERR_JEXN_ILLEGALACCESS, exn.getMessage());
				}
			}
			else {
				Init();
			}
			return ok;
		}
		public boolean Set(android.database.Cursor cur) throws StyloQException
		{
			boolean ok = true;
			if(cur != null) {
				final int fld_count = cur.getColumnCount();
				ContentValues data = new ContentValues();
				for(int i = 0; i < fld_count; i++) {
					final String fld_name = cur.getColumnName(i);
					final int fld_type = cur.getType(i);
					if(fld_type == android.database.Cursor.FIELD_TYPE_BLOB) {
						byte [] fld_value = cur.getBlob(i);
						data.put(fld_name, fld_value);
					}
					else if(fld_type == android.database.Cursor.FIELD_TYPE_STRING) {
						String fld_value = cur.getString(i);
						data.put(fld_name, (fld_value != null) ? fld_value : "");
					}
					else if(fld_type == android.database.Cursor.FIELD_TYPE_INTEGER) {
						final long fld_value = cur.getLong(i);
						data.put(fld_name, fld_value);
					}
					else if(fld_type == android.database.Cursor.FIELD_TYPE_FLOAT) {
						final double fld_value = cur.getDouble(i);
						data.put(fld_name, fld_value);
					}
				}
				ok = Set(data);
			}
			else
				Init();
			return ok;
		}
		public ContentValues Get()
		{
			ContentValues data = new ContentValues();
			final int count = FieldList.size();
			try {
				for(int i = 0; i < count; i++) {
					Field fld_info = FieldList.get(i);
					if(!fld_info.Name.equalsIgnoreCase("ID") /*|| !IdIsAutoIncr*/) {
						java.lang.reflect.Field fld = fld_info.Rf; // Cls.getDeclaredField(fld_info.Name);
						if(fld_info.Type == DataType.T_INT32())
							data.put(fld_info.Name, fld.getInt(this));
						else if(fld_info.Type == DataType.T_INT64())
							data.put(fld_info.Name, fld.getLong(this));
						else if(fld_info.Type == DataType.S_ZSTRING)
							data.put(fld_info.Name, (String)fld.get(this));
						else if(fld_info.Type == DataType.T_DOUBLE())
							data.put(fld_info.Name, fld.getDouble(this));
						else if(fld_info.Type == DataType.S_RAW) {
							//
							// На мой взгляд, очень неэффективная реализация, но я
							// пока не нашел более эффективного метода преобразования
							// массива байт в ContentValue
							//
							Object _obj = fld.get(this);
							final int bl = Array.getLength(_obj);
							if(bl > 0) {
								byte [] temp_bytes = new byte[bl];
								for(int bc = 0; bc < bl; bc++) {
									temp_bytes[bc] = Array.getByte(_obj, bc);
								}
								data.put(fld_info.Name, temp_bytes);
							}
						}
					}
				}
			}
			catch(Exception e) {
				e.printStackTrace();
			}
			return data;
		}
		protected ArrayList<Field> FieldList;
	}
	public class Cursor extends SQLiteCursor {
		private boolean UseCountTable = false;
		private String  CountTable = null;
		private int     Count = 0;
		/*public Cursor(SQLiteDatabase db, SQLiteCursorDriver driver, String editTable, SQLiteQuery query)
		{
			super(db, driver, editTable, query);
			SetCountTable(editTable);
		}*/
		public Cursor(SQLiteCursorDriver driver, String editTable, SQLiteQuery query)
		{
			super(driver, editTable, query);
			SetCountTable(editTable);
		}
		@Override
		public int getCount()
		{
			int count = 0;
			if(UseCountTable) {
				if(Count == 0) {
					android.database.Cursor temp_cur = getDatabase().rawQuery("SELECT COUNT(*) FROM " + CountTable, null);
					if(temp_cur != null) {
						if(temp_cur.moveToFirst())
							count = temp_cur.getInt(0);
						temp_cur.close();
					}
				}
				else
					count = Count;
			}
			else
				count = super.getCount();
			Count = count;
			return count;
		}
		public void SetCountTable(String countTable)
		{
			CountTable = countTable;
			UseCountTable = (CountTable != null && CountTable.length() > 0) ? true : false;
		}
	}
	public Database()
	{
		Ctx = null;
	}
	public Database(Context ctx, String name, int dbVer) throws StyloQException
	{
		Ctx = ctx;
		Name = name;
		//TableClasses
		int tbl_cls_count = 0;
		Class <?> sub_cls_list[] = getClass().getClasses();
		final int scl_count = sub_cls_list.length;
		{
			for(int i = 0; i < scl_count; i++) {
				Class cls = sub_cls_list[i];
				Class sup_cls = cls.getSuperclass();
				if(sup_cls != null) {
					String scsn = sup_cls.getSimpleName();
					if(scsn != null && scsn == "Table")
						tbl_cls_count += 1;
				}
			}
		}
		{
			TableClasses = new Class <?>[tbl_cls_count];
			for(int i = 0; i < scl_count; i++) {
				Class cls = sub_cls_list[i];
				Class sup_cls = cls.getSuperclass();
				if(sup_cls != null) {
					String scsn = sup_cls.getSimpleName();
					if(scsn != null && scsn == "Table")
						TableClasses[i] = cls;
				}
			}
		}
		{
			//final int count = TableClasses.length;
			TableNames = new String[tbl_cls_count];
			try {
				for(int i = 0; i < tbl_cls_count; i++) {
					Class cls = TableClasses[i];
					java.lang.reflect.Field fld = TableClasses[i].getField("TBL_NAME");
					TableNames[i] = (String) fld.get(TableClasses.getClass());
				}
			}
			catch(SQLiteException e) {
				throw new StyloQException(Ctx, e.getMessage(), null);
			}
			catch(Exception e) {
				throw new StyloQException(Ctx, e.getMessage(), null);
			}
		}
		DBHelper = new DatabaseHelper(Ctx, this, dbVer);
	}
	public void onCreate(SQLiteDatabase db) throws StyloQException
	{
		final int count = TableClasses.length;
		String tbl_name = "";
		try {
			for(int i = 0; i < count; i++) {
				java.lang.reflect.Field /*fld*/sql_op_create = TableClasses[i].getField("CREATE_SQL");
				java.lang.reflect.Field fld_tbl_name = TableClasses[i].getField("TBL_NAME");
				String sql = (String)sql_op_create.get(TableClasses.getClass());
				tbl_name = (String)fld_tbl_name.get(TableClasses.getClass());
				db.execSQL(sql);
			}
		} catch(IllegalAccessException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_ILLEGALACCESS, exn.getMessage());
		} catch(NoSuchFieldException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_NOSUCHFIELD, exn.getMessage());
		}
		/*catch(Exception e) {
			throw new StyloQException("Ошибка создания таблицы " + tbl_name + " (" + e.getMessage() + ")");
		}*/
	}
	public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) throws StyloQException
	{
		String rep_tbl_name = ""; // Наименование таблицы для отчета об ошибке
		Log.w(this.getClass().getSimpleName(), "Upgrading database from version " + oldVersion
				+ " to " + newVersion + ", which will destroy all old data");
		final int count = TableClasses.length;
		try {
			for(int i = 0; i < count; i++) {
				java.lang.reflect.Field fld = TableClasses[i].getField("TBL_NAME");
				String tbl_name = (String)fld.get(TableClasses.getClass());
				rep_tbl_name = tbl_name;
				db.execSQL("DROP TABLE IF EXISTS " + tbl_name);
			}
			onCreate(db);
		} catch(IllegalAccessException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_ILLEGALACCESS, exn.getMessage());
		} catch(NoSuchFieldException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_NOSUCHFIELD, exn.getMessage());
		}
		/*catch(Exception e) {
			throw new StyloQException("Ошибка обновления таблицы " + rep_tbl_name + " (" + e.getMessage() + ")");
		}*/
	}
	public SQLiteDatabase GetHandle() { return DB; }
	public boolean IsTableExists(String tableName)
	{
		boolean r = false;
		final int count = TableNames.length;
		for(int i = 0; r == false && i < count; i++)
			if(tableName.equalsIgnoreCase(TableNames[i]))
				r = true;
		return r;
	}
	public Table CreateTable(String tableName)
	{
		Table tbl = null;
		final int count = TableClasses.length;
		try {
			for(int i = 0; tbl == null && i < count; i++) {
				String tbl_name = TableClasses[i].getSimpleName();
				if(tbl_name.equalsIgnoreCase(tableName)) {
					tbl = (Table)TableClasses[i].newInstance();
					tbl.Init(Ctx, tableName);
				}
			}
		}
		catch(Exception e) {
			StyloQApp.SetLastError(Ctx, 0, "Таблица '%s' не найдена в базе данных", tableName);
			tbl = null; // @v11.7.5
		}
		return tbl;
	}
	public int DropTable(String tableName)
	{
		int ok = -1;
		final int count = TableClasses.length;
		try {
			for(int i = 0; ok < 0 && i < count; i++) {
				java.lang.reflect.Field fld = TableClasses[i].getField("TBL_NAME");
				String tbl_name = (String)fld.get(TableClasses.getClass());
				if(tbl_name.equalsIgnoreCase(tableName)) {
					java.lang.reflect.Field sql_fld = TableClasses[i].getField("CREATE_SQL");
					String sql = (String)sql_fld.get(TableClasses.getClass());
					DB.execSQL("DROP TABLE IF EXISTS " + tableName);
					DB.execSQL(sql);
					ok = 1;
				}
			}
		}
		catch(SQLiteException e) {
			StyloQApp.SetLastError(Ctx, 0, e.getMessage(), null);
			ok = 0;
		}
		catch(Exception e) {
			StyloQApp.SetLastError(Ctx, 0, "Ошибка создания таблицы %s", tableName);
			ok = 0;
		}
		return ok;
	}
	public void CreateTableInDb(SQLiteDatabase db, String tblName, boolean doDropBeforehand) throws SQLiteException
	{
		final int count = TableClasses.length;
		String tbl_name = "";
		if(db == null)
			db = DB;
		try {
			for(int i = 0; i < count; i++) {
				java.lang.reflect.Field fld_tbl_name = TableClasses[i].getField("TBL_NAME");
				tbl_name = (String)fld_tbl_name.get(TableClasses.getClass());
				if(tbl_name.equalsIgnoreCase(tblName)) {
					java.lang.reflect.Field fld = TableClasses[i].getField("CREATE_SQL");
					String sql = (String)fld.get(TableClasses.getClass());
					if(doDropBeforehand)
						DropTable(tblName);
					db.execSQL(sql);
				}
			}
		}
		catch(Exception e) {
			throw new SQLiteException("Ошибка создания таблицы " + tblName + ". Доп. инфо: " + e.getMessage());
		}
	}
	public Database Open()
	{
		try {
			Close();
			/*if(ReadOnly)
				DB = DBHelper.getReadableDatabase();
			else*/
				DB = DBHelper.getWritableDatabase();
		}
		catch(SQLException e) {
			StyloQApp.SetLastError(Ctx, 0, "Ошибка отрытия базы данных %s", e.getMessage());
		}
		return this;
	}
	public void Close()
	{
		if(DB != null)
			DB.close();
		if(DBHelper != null)
			DBHelper.close();
	}
	public boolean IsOpen() { return (DB != null) ? DB.isOpen() : false; }
	public static class Transaction {
		public Transaction(@NotNull Database ownerDb, boolean useTa)
		{
			OwnerDb = ownerDb;
			Started = false;
			UseTa = useTa;
			if(UseTa) {
				if(OwnerDb.Internal_StartTransaction())
					Started = true;
			}
		}
		public boolean Commit()
		{
			boolean result = true;
			if(UseTa && Started) {
				result = OwnerDb.Internal_CommitWork();
				Started = false;
			}
			return result;
		}
		public boolean Abort()
		{
			boolean result = true;
			if(UseTa && Started) {
				result = OwnerDb.Internal_RollbackWork();
				Started = false;
			}
			return result;
		}
		private Database OwnerDb;
		private boolean UseTa;
		private boolean Started;
	}
	private boolean Internal_StartTransaction()
	{
		boolean ok = false;
		if(DB != null) {
			DB.beginTransaction();
			ok = true;
		}
		return ok;
	}
	private boolean Internal_CommitWork()
	{
		boolean ok = false;
		if(DB != null) {
			DB.setTransactionSuccessful();
			DB.endTransaction();
			ok = true;
		}
		return ok;
	}
	private boolean Internal_RollbackWork()
	{
		boolean ok = false;
		if(DB != null) {
			DB.endTransaction();
			ok = true;
		}
		return ok;
	}
	public boolean InTransaction() {
		return (DB != null) ? DB.inTransaction() : false;
	}
	public long InsertRec(Table tbl, Record rec)
	{
		long id = -1;
		String add_info = (tbl != null) ? tbl.GetName() : "";
		try {
			id = tbl.Insert(rec);
		} catch(SQLiteException sqle) {
			id = -1;
			add_info += " " + sqle.getMessage();
		} catch(Exception e) {
			id = -1;
			add_info += " " + e.getMessage();
		}
		if(id == -1)
			StyloQApp.SetLastError(Ctx, R.string.sqerr_db_insertfault, null, add_info);
		return id;
	}
	public long InsertRec(Table tbl, ContentValues rec)
	{
		long id = -1;
		String add_info = (tbl != null) ? tbl.GetName() : "";
		try {
			id = tbl.Insert(rec);
		} catch(SQLiteException sqle) {
			id = -1;
			add_info += " " + sqle.getMessage();
		} catch(Exception e) {
			id = -1;
			add_info += " " + e.getMessage();
		}
		if(id == -1)
			StyloQApp.SetLastError(Ctx, R.string.sqerr_db_insertfault, null, add_info);
		return id;
	}
	public int UpdateRec(Table tbl, long id, Record rec)
	{
		int ok = -1;
		String add_info = (tbl != null) ? tbl.GetName() : "";
		try {
			ok = tbl.UpdateByID(id, rec);
		} catch(SQLiteException sqle) {
			ok = 0;
			add_info += " " + sqle.getMessage();
		} catch(Exception e) {
			ok = 0;
			add_info += " " + e.getMessage();
		}
		if(ok == 0)
			StyloQApp.SetLastError(Ctx, R.string.sqerr_db_updatefault, null, add_info);
		return ok;
	}
	public long RemoveRec(Table tbl, long id)
	{
		int ok = -1;
		String add_info = (tbl != null) ? tbl.GetName() : "";
		try {
			ok = tbl.RemoveByID(id);
		} catch(SQLiteException sqle) {
			ok = 0;
			add_info += " " + sqle.getMessage();
		} catch(Exception e) {
			ok = 0;
			add_info += " " + e.getMessage();
		}
		if(ok == 0)
			StyloQApp.SetLastError(Ctx, R.string.sqerr_db_deletefault, null, add_info);
		return ok;
	}
}
