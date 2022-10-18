// CommonPrereqModule.java
// Copyright (c) A.Sobolev 2022
//
package ru.petroglif.styloq;

import android.content.Context;
import android.content.Intent;
import android.text.Editable;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.TextWatcher;
import android.text.style.BackgroundColorSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import androidx.activity.OnBackPressedCallback;
import androidx.annotation.IdRes;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.viewpager2.widget.ViewPager2;

import com.google.android.material.tabs.TabLayout;
import com.google.android.material.textfield.TextInputEditText;

import org.jetbrains.annotations.NotNull;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Stack;
import java.util.UUID;

public class CommonPrereqModule {
	private static class SimpleSearchBlock {
		public static class IndexEntry {
			IndexEntry(int objType, int objID, int attr, final String text, final String displayText)
			{
				ObjType = objType;
				ObjID = objID;
				Attr = attr;
				Text = text;
				DisplayText = displayText;
			}
			int ObjType; // PPOBJ_XXX
			int ObjID;
			int Attr; // PPOBJATTR_XXX
			final String Text;
			final String DisplayText;
		}
		public static class Result {
			String Pattern;
			public ArrayList<IndexEntry> List;
			private int[] ObjTypeList;
			private int ObjTypeCount;
			private int SelectedItemIdx;
			private String SearchResultInfoText;

			Result()
			{
				ObjTypeCount = 0;
				ObjTypeList = new int[64];
				SelectedItemIdx = -1;
				SearchResultInfoText = null;
			}
			void Clear()
			{
				List = null;
				ObjTypeCount = 0;
				SelectedItemIdx = -1;
				SearchResultInfoText = null;
			}
			String GetSearchResultInfoText()
			{
				return SearchResultInfoText;
			}
			void SetSelectedItemIndex(int idx) { SelectedItemIdx = (List != null && idx >= 0 && idx < List.size()) ? idx : -1; }
			void ResetSelectedItemIndex() { SelectedItemIdx = -1; }
			int  GetSelectedItemIndex()
			{
				return SelectedItemIdx;
			}
			int  FindIndexOfItem(final IndexEntry item)
			{
				int result = -1;
				if(List != null && List.size() > 0) {
					for(int i = 0; result < 0 && i < List.size(); i++) {
						final IndexEntry se = List.get(i);
						if(se != null && se.ObjType == item.ObjType && se.ObjID == item.ObjID)
							result = i;
					}
				}
				return result;
			}
			void Add(IndexEntry entry)
			{
				if(entry != null) {
					boolean obj_type_found = false;
					for(int j = 0; !obj_type_found && j < ObjTypeCount; j++) {
						if(ObjTypeList[j] == entry.ObjType)
							obj_type_found = true;
					}
					if(!obj_type_found) {
						ObjTypeList[ObjTypeCount] = entry.ObjType;
						ObjTypeCount++;
					}
					if(List == null)
						List = new ArrayList<IndexEntry>();
					List.add(entry);
				}
			}
			ArrayList<IndexEntry> GetListByObjType(int objType)
			{
				ArrayList<IndexEntry> result = null;
				if(List != null) {
					for(IndexEntry iter : List) {
						if(iter != null && iter.ObjType == objType) {
							if(result == null)
								result = new ArrayList<IndexEntry>();
							result.add(iter);
						}
					}
				}
				return result;
			}
			final boolean IsThereObj(int objType, int objID)
			{
				boolean result = false;
				if(objType > 0 && objID > 0 && List != null) {
					for(int i = 0; !result && i < List.size(); i++) {
						IndexEntry se = List.get(i);
						if(se != null && se.ObjType == objType && se.ObjID == objID)
							result = true;
					}
				}
				return result;
			}
			public final int GetObjTypeCount()
			{
				return ObjTypeCount;
			}
			public final int GetObjTypeByIndex(int idx) { return (idx >= 0 && idx < ObjTypeCount) ? ObjTypeList[idx] : 0; }
		}
		SimpleSearchBlock()
		{
			Index = null;
			ObjTypeList = null;
			SearchPattern = null;
			SearchResult = null;
			RestrictionObjType = 0;
			RestrictionObjTypeCombonJustInited = false;
		}
		ArrayList<IndexEntry> Index;
		ArrayList<Integer> ObjTypeList; // @v11.4.10
		String SearchPattern; // @v11.4.11
		Result SearchResult;
		int   RestrictionObjType; // @v11.4.10
		boolean RestrictionObjTypeCombonJustInited; // @v11.4.10 Специальный флаг, препятствующий обработке
			// выбора ограничивающего типа объекта непосредственно после инициализации комбо-бокса.
			// Так как после выбора ограничения комбик исчезает, то это - критично.
			// Я не придумал пока более изящного решения :(
	}

	public void SearchResult_ResetSelectedItemIndex()
	{
		if(SsB.SearchResult != null)
			SsB.SearchResult.ResetSelectedItemIndex();
	}
	public int SearchResult_GetSelectedItmeIndex() { return (SsB.SearchResult != null) ? SsB.SearchResult.GetSelectedItemIndex() : -1; }
	public int SearchResult_GetObjTypeCount() { return (SsB.SearchResult != null) ? SsB.SearchResult.GetObjTypeCount() : 0; }
	//
	// Descr: Функция вызывается в ответ на касание пользователем элемента списка результатов поиска
	//
	public SLib.PPObjID SearchResult_ProcessSelection(Object itemObj)
	{
		SLib.PPObjID result = null;
		if(itemObj != null && itemObj instanceof SimpleSearchBlock.IndexEntry) {
			SimpleSearchBlock.IndexEntry se = (SimpleSearchBlock.IndexEntry)itemObj;
			// ! ev_subj.ItemIdx не согласуется простым образом с ev_subj.ItemObj из-за
			// двухярусной структуры списка.
			SsB.SearchResult.SetSelectedItemIndex(SsB.SearchResult.FindIndexOfItem(se));
			result = new SLib.PPObjID(se.ObjType, se.ObjID);
		}
		return result;
	}
	@NotNull public SLib.PPObjID SearchResult_GetSelectedOid()
	{
		SLib.PPObjID result = null;
		if(SsB.SearchResult != null) {
			final int idx = SsB.SearchResult.GetSelectedItemIndex();
			if(idx >= 0) {
				final int objtype = SsB.SearchResult.List.get(idx).ObjType;
				final int objid = SsB.SearchResult.List.get(idx).ObjID;
				if(objtype > 0 && objid > 0)
					result = new SLib.PPObjID(objtype, objid);
			}
		}
		if(result == null)
			result = new SLib.PPObjID();
		return result;
	}

	private SimpleSearchBlock SsB;
	public byte[] SvcIdent; // Получает через intent ("SvcIdent")
	public String CmdName; // Получает через intent ("CmdName")
	public String CmdDescr; // Получает через intent ("CmdDescr")
	public UUID CmdUuid;  // Получает через intent ("CmdUuid")
	public ArrayList<CliEntry> CliListData;
	public ArrayList<JSONObject> GoodsGroupListData;
	public ArrayList<BusinessEntity.Brand> BrandListData;
	public ArrayList<Document> IncomingDocListData;
	public GoodsFilt Gf;
	public RegistryFilt Rf; // @v11.5.3
	//
	// Descr: Структура инкапсулирующая общие параметры, передаваемые сервисом
	//
	public static class CommonSvcParam {
		CommonSvcParam()
		{
			BaseCurrencySymb = null;
			DefDuePeriodHour = 0;
			AgentID = 0;
			ActionFlags = 0;
			SvcOpID = 0;
			PosNodeID = 0;
			UseBarcodeSearch = false;
			UseCliDebt = false;
			BarcodeWeightPrefix = null;
			BarcodeCountPrefix = null;
		}
		void FromJson(JSONObject jsHead)
		{
			if(jsHead != null) {
				BaseCurrencySymb = jsHead.optString("basecurrency", null);
				DefDuePeriodHour = jsHead.optInt("dueperiodhr", 0); // @v11.4.8
				AgentID = jsHead.optInt("agentid", 0);
				SvcOpID = jsHead.optInt("svcopid", 0); // @v11.4.9
				PosNodeID = jsHead.optInt("posnodeid", 0);
				ActionFlags = Document.IncomingListActionsFromString(jsHead.optString("actions", null)); // @v11.4.8
				UseBarcodeSearch = jsHead.optBoolean("searchbarcode"); // @v11.5.0
				UseCliDebt = jsHead.optBoolean("useclidebt"); // @v11.5.4
				BarcodeWeightPrefix = jsHead.optString("barcodeweightprefix", null); // @v11.5.0
				BarcodeCountPrefix = jsHead.optString("barcodecountprefix", null); // @v11.5.0
			}
		}
		public String BaseCurrencySymb;
		public int DefDuePeriodHour; // @v11.4.8 Срок исполнения заказа по умолчанию (в часах). Извлекается из заголока данных по тегу "dueperiodhr"
		public int AgentID; // Если исходный документ для формирования заказов ассоциирован
			// с агентом, то в этом поле устанавливается id этого агента (field agentid)
		public int ActionFlags; // Document.actionXXX flags. Извлекается из заголока данных по тегу "actions"
		public int SvcOpID; // @v11.4.9 Вид операции для новых документов, переданный сервисом в заголовке с тегом "svcopid"
		public int PosNodeID; // Если исходный документ сформирован для indoor-обслуживания,
			// то в этом поле устанавливается id кассового узла, переданный от сервиса
		public boolean UseBarcodeSearch; // @v11.5.0 Если true, то в панели инструментов появляется иконка поиска по штрихкоду.
			// Передается сервисом с тегом "searchbarcode"
		public boolean UseCliDebt; // @v11.5.4 Если true, то для агентских заказов можно просматривать долги по клиентам
		public String BarcodeWeightPrefix; // @v11.5.0 Если строка не пустая, то трактуется как префикс весового штрихкода.
			// Передается сервисом с тегом "barcodeweightprefix". Обязанность проверки валидности префикса лежит на сервисе.
		public String BarcodeCountPrefix; // @v11.5.0 Если строка не пустая, то трактуется как префикс счетного штрихкода.
			// Передается сервисом с тегом "barcodecountprefix". Обязанность проверки валидности префикса лежит на сервисе.
	}
	//
	// Descr: Фильтр списка документов, отображаемых на вкладке Tab.tabRegistry
	//
	public static class RegistryFilt {
		RegistryFilt()
		{
			Period = null;
			Flags = 0;
			PredefPeriod = SLib.PREDEFPRD_NONE;
		}
		boolean IsEmpty()
		{
			return ((Period == null || Period.IsZero()) && Flags == 0);
		}
		public static final int fHideRejected = 0x00001;
		SLib.DateRange Period;
		int    Flags;
		int    PredefPeriod; // SLib.PREDEFPRD_XXX Только для диалога редактирования: при фильтрации не используется
	}
	private CommonSvcParam CSVCP;
	public ArrayList<WareEntry> GoodsListData;
	public ArrayList<CommonPrereqModule.TabEntry> TabList;
	private Stack<Tab> TabNavStack; // @v11.5.0
	protected OnBackPressedCallback Callback_BackButton; // @v11.5.0
	public ArrayList <Document.DisplayEntry> RegistryHList;
	public ArrayList <ProcessorEntry> ProcessorListData;
	public ArrayList <BusinessEntity.Uom> UomListData;
	private Document CurrentOrder;
	protected boolean CommitCurrentDocument_Locker;
	protected long CommitCurrentDocument_StartTm;
	private SLib.SlActivity ActivityInstance;
	private int ViewPagerResourceId;
	private int TabLayoutResourceId;
	protected boolean CurrentDocument_RemoteOp_Start()
	{
		boolean result = false;
		if(!CommitCurrentDocument_Locker) {
			CommitCurrentDocument_Locker = true;
			CommitCurrentDocument_StartTm = System.currentTimeMillis();
			result = true;
		}
		return result;
	}
	protected void CurrentDocument_RemoteOp_Finish()
	{
		CommitCurrentDocument_Locker = false;
		CommitCurrentDocument_StartTm = 0;
	}
	protected long GetCurrentDocument_RemoteOp_Duration()
	{
		return CommitCurrentDocument_Locker ? (System.currentTimeMillis() - CommitCurrentDocument_StartTm) : 0;
	}
	public static class CliEntry {
		CliEntry(JSONObject jsItem)
		{
			JsItem = jsItem;
			AddrExpandStatus = 0;
			if(JsItem != null) {
				JSONArray dlvr_loc_list = JsItem.optJSONArray("dlvrloc_list");
				if(dlvr_loc_list != null && dlvr_loc_list.length() > 0)
					AddrExpandStatus = 1;
			}
		}
		public ArrayList<JSONObject> GetDlvrLocListAsArray()
		{
			ArrayList<JSONObject> result = null;
			JSONArray dlvr_loc_list = JsItem.optJSONArray("dlvrloc_list");
			if(dlvr_loc_list != null && dlvr_loc_list.length() > 0) {
				result = new ArrayList<JSONObject>();
				try {
					for(int i = 0; i < dlvr_loc_list.length(); i++) {
						Object dlvr_loc_list_item_obj = dlvr_loc_list.get(i);
						if(dlvr_loc_list_item_obj != null && dlvr_loc_list_item_obj instanceof JSONObject)
							result.add((JSONObject) dlvr_loc_list_item_obj);
					}
				} catch(JSONException exn) {
					result = null;
				}
			}
			return result;
		}
		int AddrExpandStatus; // 0 - no addressed, 1 - addresses collapsed, 2 - addresses expanded
		JSONObject JsItem;
	}

	public enum Tab {
		tabUndef,
		tabGoodsGroups,
		tabBrands,
		tabGoods,
		tabClients,
		tabProcessors,
		tabAttendance,
		tabCurrentDocument,
		tabBookingDocument,
		tabRegistry,
		tabSearch,
		tabIncomingList,
		tabXclVerify,  // Вкладка для сканирования кодов маркировки товаров с целью проверки
		tabXclSetting, // Вкладка для сканирования кодов маркировки товаров с целью сопоставления отгружаемых марок со строками документа
	}

	public static class TabEntry {
		TabEntry(Tab id, String text, /*View*/SLib.SlFragmentStatic view)
		{
			TabId = id;
			TabText = text;
			TabView = view;
		}
		Tab TabId;
		String TabText;
		/*View*/ SLib.SlFragmentStatic TabView;
	}
	public static class TabInitEntry {
		TabInitEntry(final Tab tab, final int rc, final String title, boolean condition)
		{
			Tab = tab;
			Rc = rc;
			Title = title;
			Condition = condition;
		}
		final Tab Tab;
		final int Rc;
		final String Title;
		final boolean Condition;
	}
	private StyloQApp GetAppCtx() { return (ActivityInstance != null) ? ActivityInstance.GetAppCtx() : null; }
	protected boolean IsCurrentDocumentEmpty() { return (CurrentOrder == null || CurrentOrder.H == null); }
	protected int  GetCurrentDocumentTransferListCount() { return (CurrentOrder != null && CurrentOrder.TiList != null) ? CurrentOrder.TiList.size() : 0; }
	protected Document GetCurrentDocument()
	{
		return CurrentOrder;
	}
	protected int  GetCurrentDocumentBookingListCount() { return (CurrentOrder != null && CurrentOrder.BkList != null) ? CurrentOrder.BkList.size() : 0; }
	protected void InitCurrenDocument(int intechangeOpID) throws StyloQException
	{
		if(CurrentOrder == null) {
			StyloQApp app_ctx = GetAppCtx();
			if(app_ctx != null) {
				CurrentOrder = new Document(intechangeOpID, SvcIdent, app_ctx);
				CurrentOrder.H.BaseCurrencySymb = GetBaseCurrencySymb();
				CurrentOrder.H.OrgCmdUuid = CmdUuid;
				CurrentOrder.H.AgentID = GetAgentID(); // @v11.4.8
				CurrentOrder.H.SvcOpID = CSVCP.SvcOpID; // @v11.5.0
				if(CSVCP.DefDuePeriodHour > 0) {
					SLib.LDATETIME base_tm = CurrentOrder.H.GetNominalTimestamp();
					if(base_tm != null) {
						CurrentOrder.H.DueTime = SLib.plusdatetimesec(base_tm, CSVCP.DefDuePeriodHour * 3600);
					}
				}
				if(CSVCP.PosNodeID > 0)
					CurrentOrder.H.PosNodeID = CSVCP.PosNodeID;
			}
		}
	}
	protected void ResetCurrentDocument()
	{
		CurrentOrder = null;
	}
	protected boolean SetIncomingDocument(Document doc)
	{
		boolean ok = false;
		StyloQApp app_ctx = GetAppCtx();
		if(doc != null) {
			if(app_ctx != null) {
				CurrentOrder = Document.Copy(doc); // Важно: мы используем глубокую копию документа, а не его алиас,
					// поскольку мы будем его менять не зная однозначно будут приняты эти изменения или нет.
				CurrentOrder.H.OrgCmdUuid = CmdUuid;
				int ex_status = CurrentOrder.GetDocStatus();
				if(ex_status == 0)
					CurrentOrder.SetDocStatus(StyloQDatabase.SecStoragePacket.styloqdocstINCOMINGMOD);
				ok = true;
			}
		}
		else {
			ResetCurrentDocument();
			ok = true;
		}
		return ok;
	}
	protected boolean LoadDocument(long id)
	{
		boolean ok = false;
		StyloQApp app_ctx = GetAppCtx();
		if(id > 0 && app_ctx != null) {
			try {
				StyloQDatabase db = app_ctx.GetDB();
				if(db != null) {
					StyloQDatabase.SecStoragePacket pack = db.GetPeerEntry(id);
					if(pack != null && pack.Rec.Kind == StyloQDatabase.SecStoragePacket.kDocIncoming || pack.Rec.Kind == StyloQDatabase.SecStoragePacket.kDocOutcoming) {
						JSONObject js_doc = (pack.Pool != null) ? pack.Pool.GetJsonObject(SecretTagPool.tagRawData) : null;
						if(js_doc != null) {
							Document rd = new Document();
							if(rd.FromJsonObj(js_doc)) {
								rd.H.Flags = pack.Rec.Flags;
								// На этапе разработки было множество проблем, по этому,
								// вероятно расхождение между идентификаторами в json и в заголовке записи.
								if(rd.H.ID != pack.Rec.ID)
									rd.H.ID = pack.Rec.ID;
								if(CurrentOrder != null) {
									StoreCurrentDocument(StyloQDatabase.SecStoragePacket.styloqdocstDRAFT, StyloQDatabase.SecStoragePacket.styloqdocstDRAFT);
								}
								CurrentOrder = rd;
								ok = true;
							}
						}
					}
				}
			} catch(StyloQException exn) {
				ok = false;
			}
		}
		return ok;
	}
	protected boolean RestoreRecentIncomingModDocumentAsCurrent(/*int opID*/ArrayList <UUID> possibleDocUuidList)
	{
		boolean result = false;
		if(CurrentOrder == null) {
			StyloQApp app_ctx = GetAppCtx();
			if(app_ctx != null) {
				try {
					StyloQDatabase db = app_ctx.GetDB();
					if(db != null) {
						StyloQDatabase.SecStoragePacket rdd = db.FindRecentIncomingModDoc(StyloQDatabase.SecStoragePacket.doctypGeneric,0, null, CmdUuid, possibleDocUuidList);
						JSONObject js_doc = (rdd != null && rdd.Pool != null) ? rdd.Pool.GetJsonObject(SecretTagPool.tagRawData) : null;
						if(js_doc != null) {
							Document rd = new Document();
							if(rd.FromJsonObj(js_doc)) {
								// На этапе разработки было множество проблем, по этому,
								// вероятно расхождение между идентификаторами в json и в заголовке записи.
								if(rd.H.ID != rdd.Rec.ID)
									rd.H.ID = rdd.Rec.ID;
								rd.H.Flags = rdd.Rec.Flags;
								CurrentOrder = rd;
								result = true;
							}
						}
					}
				} catch(StyloQException exn) {
					;
				}
			}
		}
		return result;
	}
	protected void RestoreRecentDraftDocumentAsCurrent(/*int opID*/)
	{
		//public StyloQDatabase.SecStoragePacket FindRecentDraftDoc(int docType, long correspondId, byte [] ident, UUID orgCmdUuid)
		if(CurrentOrder == null) {
			StyloQApp app_ctx = GetAppCtx();
			if(app_ctx != null) {
				try {
					StyloQDatabase db = app_ctx.GetDB();
					if(db != null) {
						StyloQDatabase.SecStoragePacket rdd = db.FindRecentDraftDoc(StyloQDatabase.SecStoragePacket.doctypGeneric,0, null, CmdUuid);
						JSONObject js_doc = (rdd != null && rdd.Pool != null) ? rdd.Pool.GetJsonObject(SecretTagPool.tagRawData) : null;
						if(js_doc != null) {
							Document rd = new Document();
							if(rd.FromJsonObj(js_doc)) {
								// На этапе разработки было множество проблем, по этому,
								// вероятно расхождение между идентификаторами в json и в заголовке записи.
								if(rd.H.ID != rdd.Rec.ID)
									rd.H.ID = rdd.Rec.ID;
								rd.H.Flags = rdd.Rec.Flags;
								CurrentOrder = rd;
							}
						}
					}
				} catch(StyloQException exn) {
					;
				}
			}
		}
	}
	//
	// Descr: Отмена редактирования текущего документа, являющегося копией входящего документа.
	//   Суть операции в том, что копия документа удаляется из внутреннего хранилища, таким
	//   образом мы теряем результаты редактирования и входящий документ у сервиса никак не сможет
	//   быть нами изменен (кроме, конечно, повторного запуска цикла изменения).
	//
	protected boolean CancelCurrentIncomingDocument()
	{
		boolean result = false;
		int    turn_doc_result = -1;
		try {
			StyloQApp app_ctx = GetAppCtx();
			assert(CurrentOrder != null && CurrentOrder.H != null && CurrentOrder.H.Uuid != null);
			if(app_ctx != null && CurrentOrder != null && CurrentOrder.H != null && CurrentOrder.H.Uuid != null) {
				if(CurrentOrder.GetDocStatus() == 0)
					CurrentOrder.SetDocStatus(StyloQDatabase.SecStoragePacket.styloqdocstDRAFT);
				final int preserve_status = CurrentOrder.GetDocStatus();
				if(preserve_status == StyloQDatabase.SecStoragePacket.styloqdocstINCOMINGMOD) {
					turn_doc_result = 0;
					StyloQDatabase db = app_ctx.GetDB();
					if(db != null && SLib.GetLen(SvcIdent) > 0) {
						StyloQDatabase.SecStoragePacket svc_pack = db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, SvcIdent);
						JSONObject jsobj = CurrentOrder.ToJsonObj();
						if(svc_pack != null && jsobj != null) {
							final long svc_id = svc_pack.Rec.ID;
							final byte[] doc_ident = db.MakeDocumentStorageIdent(SvcIdent, CurrentOrder.H.Uuid);
							final int  direction = -1; // Строго входящий документ
							CurrentOrder.H.ID = 0;
							long doc_id = db.PutDocument(direction, StyloQDatabase.SecStoragePacket.doctypGeneric, CurrentOrder.H.Flags, doc_ident, svc_id, null);
							if(doc_id > 0) { // При успешном удалении документа PutDocument возвращает идентификатор последнего удаленного документа
								ResetCurrentDocument();
								turn_doc_result = 1;
							}
						}
					}
				}
			}
		} catch(StyloQException exn) {
			turn_doc_result = 0;
		}
		return (turn_doc_result > 0) ? true : ((turn_doc_result == 0) ? false : result);
	}
	private boolean StoreCurrentDocument(int reqStatus, int newStatus)
	{
		boolean result = true;
		int    turn_doc_result = -1;
		try {
			StyloQApp app_ctx = GetAppCtx();
			assert(CurrentOrder != null && CurrentOrder.H != null && CurrentOrder.H.Uuid != null);
			if(app_ctx != null && CurrentOrder != null && CurrentOrder.H != null && CurrentOrder.H.Uuid != null) {
				if(CurrentOrder.GetDocStatus() == 0)
					CurrentOrder.SetDocStatus(StyloQDatabase.SecStoragePacket.styloqdocstDRAFT);
				final int preserve_status = CurrentOrder.GetDocStatus();
				if((reqStatus == 0 || preserve_status == reqStatus) && (newStatus == 0 || CurrentOrder.SetDocStatus(newStatus))) {
					turn_doc_result = 0;
					StyloQDatabase db = app_ctx.GetDB();
					if(db != null && SLib.GetLen(SvcIdent) > 0) {
						StyloQDatabase.SecStoragePacket svc_pack = db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, SvcIdent);
						JSONObject jsobj = CurrentOrder.ToJsonObj();
						if(svc_pack != null && jsobj != null) {
							SecretTagPool doc_pool = new SecretTagPool();
							JSONObject js_query = new JSONObject();
							String js_text_doc = jsobj.toString();
							String js_text_docdecl = null;
							{
								JSONObject js_doc_decl = new JSONObject();
								js_doc_decl.put("type", "generic");
								js_doc_decl.put("format", "json");
								js_doc_decl.put("time", SLib.datetimefmt(new SLib.LDATETIME(System.currentTimeMillis()), SLib.DATF_ISO8601 | SLib.DATF_CENTURY, 0));
								js_query.put("declaration", js_doc_decl);
								js_text_docdecl = js_doc_decl.toString();
							}
							//
							// В базе данных мы сохраняем документ в виде "сырого" json (то есть только jsobj)
							// в то время как сервису передаем этот же документ вложенный в объект команды (js_query).
							// Но и то и другое вносится в пул хранения под тегом tagRawData.
							//
							SecretTagPool.DeflateStrategy ds = new SecretTagPool.DeflateStrategy(256);
							doc_pool.Put(SecretTagPool.tagRawData, js_text_doc.getBytes(StandardCharsets.UTF_8), ds);
							doc_pool.Put(SecretTagPool.tagDocDeclaration, js_text_docdecl.getBytes(StandardCharsets.UTF_8));
							//
							long svc_id = svc_pack.Rec.ID;
							byte[] doc_ident = db.MakeDocumentStorageIdent(SvcIdent, CurrentOrder.H.Uuid);
							int  direction = 0;
							if(reqStatus == StyloQDatabase.SecStoragePacket.styloqdocstINCOMINGMOD) {
								CurrentOrder.H.ID = 0; // Мы получили от сервиса документ с идентификатором, актуальным для сервиса.
									// Сохраняя этот док у себя в БД мы присваиваем ему собственный идент, сопоставлять с сервисом будем
									// по GUID'у (сервис на своей стороне обязан позабодиться о том, чтобы переданный нам документ
									// имел GUID.
								direction = -1;
							}
							else
								direction = +1;
							long doc_id = db.PutDocument(direction, StyloQDatabase.SecStoragePacket.doctypGeneric, CurrentOrder.H.Flags, doc_ident, svc_id, doc_pool);
							if(doc_id > 0) {
								assert(CurrentOrder.H.ID == 0 || CurrentOrder.H.ID == doc_id);
								if(CurrentOrder.H.ID == 0 || CurrentOrder.H.ID == doc_id) {
									CurrentOrder.H.ID = doc_id;
									turn_doc_result = 1;
								}
							}
						}
					}
				}
			}
		} catch(StyloQException exn) {
			turn_doc_result = 0;
		} catch(JSONException exn) {
			turn_doc_result = 0;
		}
		return (turn_doc_result > 0) ? true : ((turn_doc_result == 0) ? false : result);
	}
	protected void OnCurrentDocumentModification()
	{
		if(!IsCurrentDocumentEmpty()) {
			final int status = CurrentOrder.GetDocStatus();
			int new_status = 0;
			if(status == StyloQDatabase.SecStoragePacket.styloqdocstINCOMINGMOD)
				new_status = StyloQDatabase.SecStoragePacket.styloqdocstINCOMINGMOD;
			else if(status == StyloQDatabase.SecStoragePacket.styloqdocstDRAFT)
				new_status = StyloQDatabase.SecStoragePacket.styloqdocstDRAFT;
			if(new_status != 0)
				StoreCurrentDocument(status, new_status);
		}
	}
	protected void OnCurrentDocumentModification_Incoming()
	{
		if(!IsCurrentDocumentEmpty())
			StoreCurrentDocument(StyloQDatabase.SecStoragePacket.styloqdocstINCOMINGMOD, StyloQDatabase.SecStoragePacket.styloqdocstINCOMINGMOD);
	}
	protected boolean UpdateMemoInCurrentDocument(String memo)
	{
		boolean result = false;
		if(!IsCurrentDocumentEmpty()) {
			int len1 = SLib.GetLen(memo);
			int len2 = SLib.GetLen(CurrentOrder.H.Memo);
			if(len1 != len2 || (len1 > 0 && !memo.equals(CurrentOrder.H.Memo))) {
				CurrentOrder.H.Memo = memo;
				result = true;
			}
		}
		return result;
	}
	protected boolean UpdateTransferItemQttyInCurrentDocument(final Document.TransferItem srcData)
	{
		boolean result = false;
		if(srcData != null && srcData.Set != null && GetCurrentDocumentTransferListCount() > 0) {
			for(int i = 0; !result && i < GetCurrentDocumentTransferListCount(); i++) {
				Document.TransferItem ti = CurrentOrder.TiList.get(i);
				if(ti.RowIdx == srcData.RowIdx) {
					if(srcData.Set.Qtty > 0)
						CurrentOrder.TiList.get(i).Set.Qtty = srcData.Set.Qtty;
					else
						CurrentOrder.TiList.remove(i);
					result = true;
					break;
				}
			}
			if(result)
				OnCurrentDocumentModification();
		}
		return result;
	}
	protected boolean AddTransferItemToCurrentDocument(Document.TransferItem item)
	{
		boolean result = false;
		try {
			if(item != null && item.GoodsID > 0 && item.Set != null && item.Set.Qtty > 0.0) {
				WareEntry goods_item = FindGoodsItemByGoodsID(item.GoodsID);
				//double price = goods_item.JsItem.optDouble("price", 0.0);
				double price = goods_item.Item.Price;
				int    interchange_op_id = 0;
				int    posnode_id = GetPosNodeID();
				if(posnode_id > 0)
					interchange_op_id = SLib.sqbdtCCheck;
				else
					interchange_op_id = SLib.PPEDIOP_ORDER;
				InitCurrenDocument(interchange_op_id);
				Document.TransferItem ti = item;
				ti.Set.Price = price;
				int max_row_idx = 0;
				boolean merged = false;
				if(CurrentOrder.TiList != null) {
					for(int i = 0; !merged && i < CurrentOrder.TiList.size(); i++) {
						Document.TransferItem iter_ti = CurrentOrder.TiList.get(i);
						merged = iter_ti.Merge(ti);
						if(max_row_idx < iter_ti.RowIdx)
							max_row_idx = iter_ti.RowIdx;
					}
				}
				if(!merged) {
					ti.RowIdx = max_row_idx + 1;
					if(CurrentOrder.TiList == null)
						CurrentOrder.TiList = new ArrayList<Document.TransferItem>();
					CurrentOrder.TiList.add(ti);
				}
				SetTabVisibility(Tab.tabCurrentDocument, View.VISIBLE);
				//NotifyCurrentOrderChanged();
				OnCurrentDocumentModification();
				result = true;
			}
		} catch(StyloQException exn) {
			;
		}
		return result;
	}
	protected SLib.STimeChunkArray PutBookingItemToCurrentDocument(Document.BookingItem item)
	{
		SLib.STimeChunkArray result = null;
		try {
			if(item != null) {
				InitCurrenDocument(SLib.sqbdtSvcReq);
				if(CurrentOrder.BkList == null)
					CurrentOrder.BkList = new ArrayList<Document.BookingItem>();
				CurrentOrder.BkList.clear();
				/*
				Document.BookingItem bk_item = new Document.BookingItem();
				bk_item.GoodsID = goods_id;
				bk_item.PrcID = prc_id;
				bk_item.RowIdx = 1;
				bk_item.ReqTime = new SLib.LDATETIME(dt, start_tm);
				bk_item.EstimatedDurationSec = CPM.GetServiceDurationForPrc(prc_id, goods_id);
				if(bk_item.EstimatedDurationSec <= 0)
					bk_item.EstimatedDurationSec = 3600; // default value
				 */
				CurrentOrder.BkList.add(item);
				OnCurrentDocumentModification();
				result = GetCurrentDocumentBusyList(item.PrcID);
			}
		} catch(StyloQException exn) {
			;
		}
		return result;
	}
	protected boolean SetClientToCurrentDocument(int interchangeOpID, int cliID, int dlvrLocID, boolean forceUpdate)
	{
		boolean result = false;
		try {
			StyloQApp app_ctx = GetAppCtx();
			if(cliID > 0 && app_ctx != null) {
				JSONObject new_cli_entry = FindClientEntry(cliID);
				if(new_cli_entry != null) {
					if(CurrentOrder == null) {
						InitCurrenDocument(interchangeOpID);
						CurrentOrder.H.ClientID = cliID;
						CurrentOrder.H.DlvrLocID = dlvrLocID;
						CurrentOrder.H.BaseCurrencySymb = CSVCP.BaseCurrencySymb;
						result = true;
					}
					else {
						if(CurrentOrder.H.ClientID == 0) {
							CurrentOrder.H.ClientID = cliID;
							CurrentOrder.H.DlvrLocID = dlvrLocID;
							result = true;
						}
						else if(CurrentOrder.H.ClientID == cliID) {
							if(dlvrLocID != CurrentOrder.H.DlvrLocID) {
								CurrentOrder.H.DlvrLocID = dlvrLocID;
								result = true;
							}
						}
						else {
							// Здесь надо как-то умнО обработать изменение контрагента
							if(forceUpdate) {
								CurrentOrder.H.ClientID = cliID;
								CurrentOrder.H.DlvrLocID = dlvrLocID;
								result = true;
							}
							else {
								final int st = CurrentOrder.GetDocStatus();
								if(st == StyloQDatabase.SecStoragePacket.styloqdocstDRAFT || st == 0) {
									JSONObject prev_cli_entry = FindClientEntry(CurrentOrder.H.ClientID);
									if(prev_cli_entry != null) {
										String prev_cli_name = prev_cli_entry.optString("nm", "");
										String new_cli_name = new_cli_entry.optString("nm", "");
										String msg_addendum = prev_cli_name + " -> " + new_cli_name;
										String text_fmt = app_ctx.GetString(ppstr2.PPSTR_CONFIRMATION, ppstr2.PPCFM_STQ_CHANGEORDCLI);
										class OnResultListener implements SLib.ConfirmationListener {
											@Override public void OnResult(SLib.ConfirmationResult r)
											{
												if(r == SLib.ConfirmationResult.YES) {
													SetClientToCurrentDocument(interchangeOpID, cliID, dlvrLocID, true);
												}
											}
										}
										SLib.Confirm_YesNo(ActivityInstance, String.format(text_fmt, msg_addendum), new OnResultListener());
									}
									else {

									}
								}
								else {

								}
							}
						}
					}
					if(result)
						OnCurrentDocumentModification();
				}
				else {
					; // @err
				}

			}
		} catch(StyloQException exn) {
			result = false;
		}
		return result;
	}
	public SLib.STimeChunkArray GetCurrentDocumentBusyList(int prcID)
	{
		SLib.STimeChunkArray result = null;
		if(prcID > 0 && CurrentOrder != null && CurrentOrder.BkList != null && CurrentOrder.BkList.size() > 0) {
			for(int i = 0; i < CurrentOrder.BkList.size(); i++) {
				Document.BookingItem bi = CurrentOrder.BkList.get(i);
				if(bi != null && bi.PrcID == prcID) {
					SLib.STimeChunk tc = bi.GetEsimatedTimeChunk();
					if(tc != null) {
						if(result == null)
							result = new SLib.STimeChunkArray();
						result.add(tc);
					}
				}
			}
		}
		return result;
	}
	protected double GetAmountOfCurrentDocument()
	{
		double result = 0.0;
		if(CurrentOrder != null) {
			if(CurrentOrder.TiList != null) {
				for(int i = 0; i < CurrentOrder.TiList.size(); i++) {
					Document.TransferItem ti = CurrentOrder.TiList.get(i);
					if(ti != null && ti.Set != null)
						result += Math.abs(ti.Set.Qtty * ti.Set.Price);
				}
			}
			if(CurrentOrder.BkList != null) {
				for(int i = 0; i < CurrentOrder.BkList.size(); i++) {
					Document.BookingItem bi = CurrentOrder.BkList.get(i);
					if(bi != null && bi.Set != null)
						result += Math.abs(bi.Set.Qtty * bi.Set.Price);
				}
			}
		}
		return result;
	}
	protected boolean CommitCurrentDocument()
	{
		boolean ok = false;
		StyloQApp app_ctx = GetAppCtx();
		if(app_ctx != null) {
			if(CurrentDocument_RemoteOp_Start()) {
				if(CurrentOrder != null) {
					if(CurrentOrder.Finalize()) {
						// @v11.5.0 {
						if(CurrentOrder.H.SvcOpID == 0)
							CurrentOrder.H.SvcOpID = CSVCP.SvcOpID;
						// } @v11.5.0
						boolean is_err = false;
						if(GetAgentID() > 0) {
							if(CurrentOrder.H.ClientID == 0) {
								app_ctx.DisplayError(ActivityInstance, ppstr2.PPERR_STQ_BUYERNEEDED, 0);
								is_err = true;
							}
						}
						if(!is_err) {
							final int s = CurrentOrder.GetDocStatus();
							int direction = +1; // outcoming
							if(s == StyloQDatabase.SecStoragePacket.styloqdocstDRAFT)
								CurrentOrder.SetAfterTransmitStatus(StyloQDatabase.SecStoragePacket.styloqdocstWAITFORAPPROREXEC);
							else if(s == StyloQDatabase.SecStoragePacket.styloqdocstINCOMINGMOD) { // @v11.4.9
								CurrentOrder.SetAfterTransmitStatus(StyloQDatabase.SecStoragePacket.styloqdocstINCOMINGMODACCEPTED);
								direction = -1; // incoming
							}
							// @todo Здесь еще долго со статусами разбираться придется!
							StyloQApp.PostDocumentResult result = app_ctx.RunSvcPostDocumentCommand(SvcIdent, CSVCP.ActionFlags, direction, CurrentOrder, ActivityInstance);
							ok = result.PostResult;
							if(ok) {
								;
							}
						}
					}
				}
				if(!ok)
					CurrentDocument_RemoteOp_Finish();
			}
		}
		return ok;
	}
	protected boolean CancelCurrentDocument()
	{
		boolean ok = false;
		if(CurrentDocument_RemoteOp_Start()) {
			ok = Helper_CancelCurrentDocument(false);
			if(!ok)
				CurrentDocument_RemoteOp_Finish();
		}
		return ok;
	}
	private boolean Helper_CancelCurrentDocument(boolean force)
	{
		boolean ok = true;
		StyloQApp app_ctx = GetAppCtx();
		if(app_ctx != null) {
			if(CurrentOrder != null) {
				if(CurrentOrder.Finalize()) {
					// @v11.5.0 {
					if(CurrentOrder.H.SvcOpID == 0)
						CurrentOrder.H.SvcOpID = CSVCP.SvcOpID;
					// } @v11.5.0
					final int st = CurrentOrder.GetDocStatus();
					if(st == 0 || st == StyloQDatabase.SecStoragePacket.styloqdocstDRAFT) {
						// Такой документ просто удаляем
						if(!force) {
							class OnResultListener implements SLib.ConfirmationListener {
								@Override
								public void OnResult(SLib.ConfirmationResult r)
								{
									if(r == SLib.ConfirmationResult.YES)
										Helper_CancelCurrentDocument(true);
									else
										CurrentDocument_RemoteOp_Finish();
								}
							}
							String text = app_ctx.GetString(ppstr2.PPSTR_CONFIRMATION, ppstr2.PPCFM_STQ_RMVORD_DRAFT);
							SLib.Confirm_YesNo(ActivityInstance, text, new OnResultListener());
						}
						else {
							boolean local_err = false;
							if(CurrentOrder.H.ID > 0) {
								//SetDocStatus(StyloQDatabase.SecStoragePacket.styloqdocstDRAFT); // Новый док автоматом является draft-документом
								boolean r = StoreCurrentDocument(StyloQDatabase.SecStoragePacket.styloqdocstDRAFT, StyloQDatabase.SecStoragePacket.styloqdocstCANCELLEDDRAFT);
								//
								// Далее эмулируем ответ от сервиса, ибо в данном случае никакого обращения не было (драфт-документ у сервиса не бывал)
								//
								StyloQApp.InterchangeResult subj = null;
								if(r) {
									subj = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.SUCCESS, SvcIdent, "", null);
								}
								else {
									subj = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.ERROR, SvcIdent, "", null);
								}
								subj.OriginalCmdItem = new StyloQCommand.Item();
								subj.OriginalCmdItem.Name = "CancelDocument";
								ActivityInstance.HandleEvent(SLib.EV_SVCQUERYRESULT, null, subj);
							}
						}
					}
					else if(st == StyloQDatabase.SecStoragePacket.styloqdocstWAITFORAPPROREXEC) {
						StyloQApp.InterchangeResult subj = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.ERROR, SvcIdent, "Function isn't supported", null);
						subj.OriginalCmdItem = new StyloQCommand.Item();
						subj.OriginalCmdItem.Name = "CancelDocument";
						ActivityInstance.HandleEvent(SLib.EV_SVCQUERYRESULT, null, subj);
					}
					else if(st == StyloQDatabase.SecStoragePacket.styloqdocstAPPROVED) {
						// Надо отправить уведомление об отмене сервису
						StyloQApp.InterchangeResult subj = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.ERROR, SvcIdent, "Function isn't supported", null);
						subj.OriginalCmdItem = new StyloQCommand.Item();
						subj.OriginalCmdItem.Name = "CancelDocument";
						ActivityInstance.HandleEvent(SLib.EV_SVCQUERYRESULT, null, subj);
					}
				}
			}
		}
		return ok;
	}
	protected void DrawCurrentDocumentRemoteOpIndicators()
	{
		if(ActivityInstance != null) {
			long ellapsed_ms = GetCurrentDocument_RemoteOp_Duration();
			View tv_ind_et = ActivityInstance.findViewById(R.id.CTL_DOCUMENT_IND_EXECUTETIME);
			View tv_ind_img = ActivityInstance.findViewById(R.id.CTL_DOCUMENT_IND_STATUS);
			if(ellapsed_ms > 0) {
				if(tv_ind_et != null && tv_ind_et instanceof TextView) {
					final int sec = (int) (ellapsed_ms / 1000);
					final int h = (int) (sec / 3600);
					String timewatch_text = ((h > 0) ? Integer.toString(h) + ":" : "") + String.format("%02d:%02d", (sec % 3600) / 60, (sec % 60));
					if(tv_ind_et.getVisibility() != View.VISIBLE)
						tv_ind_et.setVisibility(View.VISIBLE);
					((TextView)tv_ind_et).setText(timewatch_text);
				}
				{
					if(tv_ind_img != null && tv_ind_img instanceof ImageView) {
						if(tv_ind_img.getVisibility() != View.VISIBLE)
							tv_ind_img.setVisibility(View.VISIBLE);
						((ImageView)tv_ind_img).setImageResource(R.drawable.ic_stopwatch);
					}
				}
			}
			else {
				if(tv_ind_et != null && tv_ind_et.getVisibility() == View.VISIBLE)
					tv_ind_et.setVisibility(View.GONE);
				if(tv_ind_img != null && tv_ind_img.getVisibility() == View.VISIBLE)
					tv_ind_img.setVisibility(View.GONE);
			}
		}
	}
	public boolean HasPrcInCurrentOrder(int prcID)
	{
		boolean result = false;
		if(CurrentOrder != null) {
			if(CurrentOrder.BkList != null) {
				for(int i = 0; !result && i < CurrentOrder.BkList.size(); i++) {
					Document.BookingItem bi = CurrentOrder.BkList.get(i);
					if(bi != null && bi.PrcID == prcID)
						result = true;
				}
			}
		}
		return result;
	}
	public boolean HasGoodsInCurrentOrder(int goodsID)
	{
		boolean result = false;
		if(CurrentOrder != null) {
			if(CurrentOrder.TiList != null) {
				for(int i = 0; !result && i < CurrentOrder.TiList.size(); i++) {
					Document.TransferItem ti = CurrentOrder.TiList.get(i);
					if(ti != null && ti.GoodsID == goodsID)
						result = true;
				}
			}
			if(!result) {
				if(CurrentOrder.BkList != null) {
					for(int i = 0; !result && i < CurrentOrder.BkList.size(); i++) {
						Document.BookingItem bi = CurrentOrder.BkList.get(i);
						if(bi != null && bi.GoodsID == goodsID)
							result = true;
					}
				}
			}
		}
		return result;
	}
	protected Document.TransferItem SearchGoodsItemInCurrentOrderTi(int goodsID)
	{
		Document.TransferItem result = null;
		if(CurrentOrder != null && CurrentOrder.TiList != null) {
			for(Document.TransferItem iter : CurrentOrder.TiList) {
				if(iter != null && iter.GoodsID == goodsID) {
					result = iter;
					break;
				}
			}
		}
		return result;
	}
	protected double GetGoodsQttyInCurrentDocument(int goodsID)
	{
		double result = 0.0;
		if(CurrentOrder != null && CurrentOrder.TiList != null) {
			for(Document.TransferItem iter : CurrentOrder.TiList) {
				if(iter != null && iter.GoodsID == goodsID)
					result += Math.abs(iter.Set.Qtty);
			}
		}
		return result;
	}
	public static class WareEntry {
		WareEntry(JSONObject jsItem)
		{
			//JsItem = jsItem;
			Item = new BusinessEntity.Goods();
			Item.FromJsonObj(jsItem);
			PrcExpandStatus = 0;
			PrcPrice = 0.0;
		}
		WareEntry(BusinessEntity.Goods srcItem)
		{
			Item = srcItem;
			PrcExpandStatus = 0;
			PrcPrice = 0.0;
		}
		//JSONObject JsItem;
		BusinessEntity.Goods Item;
		int   PrcExpandStatus; // 0 - no processors, 1 - processors collapsed, 2 - processors expanded
		double PrcPrice; // Специфическая цена товара для конкретного процессора
	}
	public static class ProcessorEntry {
		ProcessorEntry(JSONObject jsItem)
		{
			JsItem = jsItem;
			GoodsExpandStatus = 0;
		}
		JSONObject JsItem;
		int   GoodsExpandStatus; // 0 - no goods, 1 - goods collapsed, 2 - goods expanded
	}
	public void InitSimpleIndex()
	{
		if(SsB.Index == null)
			SsB.Index = new ArrayList<SimpleSearchBlock.IndexEntry>();
		else
			SsB.Index.clear();
		SsB.ObjTypeList = null;
	}
	public void AddSimpleIndexEntry(int objType, int objID, int attr, final String text, final String displayText)
	{
		//SimpleSearchIndexEntry(int objType, int objID, int attr, final String text, final String displayText)
		if(SLib.GetLen(text) > 0) {
			SsB.Index.add(new SimpleSearchBlock.IndexEntry(objType, objID, attr, text.toLowerCase(), displayText));
			if(objType > 0) {
				if(SsB.ObjTypeList == null) {
					SsB.ObjTypeList = new ArrayList<Integer>();
					SsB.ObjTypeList.add(objType);
				}
				else {
					boolean found = false;
					for(Integer iter : SsB.ObjTypeList) {
						if(iter != null && iter.intValue() == objType) {
							found = true;
							break;
						}
					}
					if(!found)
						SsB.ObjTypeList.add(objType);
				}
			}
		}
	}
	public void AddGoodsToSimpleIndex()
	{
		if(GoodsListData != null) {
			for(int i = 0; i < GoodsListData.size(); i++) {
				WareEntry ware_item = GoodsListData.get(i);
				if(ware_item != null && ware_item.Item != null) {
					int id = ware_item.Item.ID;
					if(id > 0) {
						final String nm = ware_item.Item.Name;
						AddSimpleIndexEntry(SLib.PPOBJ_GOODS, id, SLib.PPOBJATTR_NAME, nm, null);
						if(ware_item.Item.CodeList != null && ware_item.Item.CodeList.size() > 0) {
							for(int j = 0; j < ware_item.Item.CodeList.size(); j++) {
								BusinessEntity.GoodsCode code_item = ware_item.Item.CodeList.get(j);
								if(code_item != null && SLib.GetLen(code_item.Code) > 0)
									AddSimpleIndexEntry(SLib.PPOBJ_GOODS, id, SLib.PPOBJATTR_CODE, code_item.Code, nm);
							}
						}
					}
				}
			}
		}
	}
	public void AddGoodsGroupsToSimpleIndex()
	{
		if(GoodsGroupListData != null) {
			for(int i = 0; i < GoodsGroupListData.size(); i++) {
				JSONObject js_item = GoodsGroupListData.get(i);
				if(js_item != null) {
					final int id = js_item.optInt("id", 0);
					if(id > 0) {
						String nm = js_item.optString("nm");
						AddSimpleIndexEntry(SLib.PPOBJ_GOODSGROUP, id, SLib.PPOBJATTR_NAME, nm, null);
					}
				}
			}
		}
	}
	public void AddBrandsToSimpleIndex()
	{
		if(BrandListData != null) {
			for(int i = 0; i < BrandListData.size(); i++) {
				BusinessEntity.Brand item = BrandListData.get(i);
				if(item != null) {
					int id = item.ID;
					if(id > 0 && SLib.GetLen(item.Name) > 0)
						AddSimpleIndexEntry(SLib.PPOBJ_BRAND, id, SLib.PPOBJATTR_NAME, item.Name, null);
				}
			}
		}
	}
	public boolean SearchInSimpleIndex(String pattern)
	{
		final int min_pattern_len = 4;
		final int min_pattern_len_code = 5;
		final int min_pattern_len_ruinn = 8;
		final int min_pattern_len_rukpp = 6;
		final int max_result_count = 1000;
		boolean result = false;
		StyloQApp app_ctx = GetAppCtx();
		if(app_ctx != null) {
			if(SsB.SearchResult == null)
				SsB.SearchResult = new SimpleSearchBlock.Result();
			SsB.SearchResult.Clear();
			SsB.SearchResult.Pattern = pattern;
			if(SsB.Index != null && SLib.GetLen(pattern) >= min_pattern_len) {
				pattern = pattern.toLowerCase();
				for(int i = 0; i < SsB.Index.size(); i++) {
					SimpleSearchBlock.IndexEntry entry = SsB.Index.get(i);
					if(entry != null && (SsB.RestrictionObjType == 0 || SsB.RestrictionObjType == entry.ObjType) && SLib.GetLen(entry.Text) > 0 && entry.Text.indexOf(pattern) >= 0) {
						boolean skip = false;
						if(entry.Attr == SLib.PPOBJATTR_CODE && pattern.length() < min_pattern_len_code)
							skip = true;
						else if(entry.Attr == SLib.PPOBJATTR_RUINN && pattern.length() < min_pattern_len_ruinn)
							skip = true;
						else if(entry.Attr == SLib.PPOBJATTR_RUKPP && pattern.length() < min_pattern_len_rukpp)
							skip = true;
						if(!skip) {
							if(SsB.SearchResult.List != null && SsB.SearchResult.List.size() >= max_result_count) {
								// Если количество результатов превышает некий порог, то считаем поиск безуспешным - пусть
								// клиент вводит что-то более длинное для релевантности результата
								SsB.SearchResult.Clear();
								String fmt_buf = app_ctx.GetString(ppstr2.PPSTR_TEXT, ppstr2.PPTXT_SMPLSRCHRESULT_TOOMANYRESULTS);
								if(SLib.GetLen(fmt_buf) > 0)
									SsB.SearchResult.SearchResultInfoText = String.format(fmt_buf, Integer.toString(max_result_count));
								result = false;
								break;
							}
							else {
								SsB.SearchResult.Add(entry);
								result = true;
							}
						}
					}
				}
				if(result) {
					String fmt_buf = app_ctx.GetString(ppstr2.PPSTR_TEXT, ppstr2.PPTXT_SMPLSRCHRESULT_SUCCESS);
					if(SLib.GetLen(fmt_buf) > 0)
						SsB.SearchResult.SearchResultInfoText = String.format(fmt_buf, Integer.toString(SsB.SearchResult.List.size()));
				}
			}
			else {
				String fmt_buf = app_ctx.GetString(ppstr2.PPSTR_TEXT, ppstr2.PPTXT_SMPLSRCHRESULT_TOOSHRTPATTERN);
				if(SLib.GetLen(fmt_buf) > 0)
					SsB.SearchResult.SearchResultInfoText = String.format(fmt_buf, Integer.toString(min_pattern_len_code));
			}
		}
		return result;
	}
	public String GetSimpleSearchResultPattern() { return (SsB.SearchResult != null) ? SsB.SearchResult.Pattern : null; }
	public final boolean IsObjInSearchResult(int objType, int objID) { return (SsB.SearchResult != null) ? SsB.SearchResult.IsThereObj(objType, objID) : false; }
	public CommonPrereqModule(SLib.SlActivity activityInstance)
	{
		SvcIdent = null;
		CmdName = null;
		CmdDescr = null;
		CmdUuid = null;
		GoodsGroupListData = null;
		BrandListData = null;
		GoodsListData = null;
		IncomingDocListData = null;
		CSVCP = new CommonSvcParam();
		Gf = null;
		Rf = null; // @v11.5.3
		CurrentOrder = null;
		RegistryHList = null;
		ProcessorListData = null;
		UomListData = null;
		CommitCurrentDocument_Locker = false;
		ActivityInstance = activityInstance;
		ViewPagerResourceId = 0;
		TabLayoutResourceId = 0;
		SsB = new SimpleSearchBlock();
		TabNavStack = new Stack<Tab>();
		Callback_BackButton = null;
	}
	String GetBaseCurrencySymb() { return CSVCP.BaseCurrencySymb; }
	int   GetDefDuePeriodHour() { return CSVCP.DefDuePeriodHour; }
	int   GetAgentID() { return CSVCP.AgentID; }
	int   GetPosNodeID() { return CSVCP.PosNodeID; }
	int   GetActionFlags() { return CSVCP.ActionFlags; }
	boolean GetOption_UseCliDebt() { return CSVCP.UseCliDebt; }
	public void GetAttributesFromIntent(Intent intent)
	{
		if(intent != null) {
			SvcIdent = intent.getByteArrayExtra("SvcIdent");
			CmdName = intent.getStringExtra("CmdName");
			CmdDescr = intent.getStringExtra("CmdDescr");
			CmdUuid = SLib.strtouuid(intent.getStringExtra("CmdUuid"));
		}
	}
	public void SetupActivity(StyloQDatabase db, int viewPagerResourceId, int tabLayoutResourceId) throws StyloQException
	{
		if(ActivityInstance != null && db != null) {
			ViewPagerResourceId = viewPagerResourceId;
			TabLayoutResourceId = tabLayoutResourceId;
			String title_text = null;
			String blob_signature = null;
			if(SLib.GetLen(SvcIdent) > 0) {
				StyloQDatabase.SecStoragePacket svc_packet = db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, SvcIdent);
				String svc_name = null;
				if(svc_packet != null) {
					StyloQFace face = svc_packet.GetFace();
					if(face != null) {
						blob_signature = face.Get(StyloQFace.tagImageBlobSignature, 0);
						svc_name = svc_packet.GetSvcName(face);
					}
				}
				if(SLib.GetLen(svc_name) > 0)
					SLib.SetCtrlString(ActivityInstance, R.id.CTL_PAGEHEADER_SVCTITLE, svc_name);
			}
			if(SLib.GetLen(CmdName) > 0)
				title_text = CmdName;
			if(SLib.GetLen(CmdDescr) > 0)
				title_text = (SLib.GetLen(title_text) > 0) ? (title_text + "\n" + CmdDescr) : CmdDescr;
			if(SLib.GetLen(title_text) > 0)
				SLib.SetCtrlString(ActivityInstance, R.id.CTL_PAGEHEADER_TOPIC, title_text);
			SLib.SetupImage(ActivityInstance, ActivityInstance.findViewById(R.id.CTLIMG_PAGEHEADER_SVC), blob_signature, false);
		}
	}
	protected Object OnEvent_CreateFragment(Object subj)
	{
		Object result = null;
		if(subj instanceof Integer) {
			int item_idx = (Integer)subj;
			if(TabList != null && item_idx >= 0 && item_idx < TabList.size()) {
				TabEntry cur_entry = (TabEntry)TabList.get(item_idx);
				if(cur_entry.TabView != null)
					result = cur_entry.TabView;
			}
		}
		return result;
	}
	public TabEntry SearchTabEntry(@IdRes int viewPagerRcId, Tab tab)
	{
		TabEntry result = null;
		if(ActivityInstance != null && tab != Tab.tabUndef) {
			View v = ActivityInstance.findViewById(viewPagerRcId);
			if(v != null && v instanceof ViewPager2) {
				for(int tidx = 0; tidx < TabList.size(); tidx++) {
					if(TabList.get(tidx).TabId == tab)
						result = TabList.get(tidx);
				}
			}
		}
		return result;
	}
	//
	// Descr: Предпринимает необходимые общие действия в ответ на зименение текущей закладки ViewPager2
	// ARG(subj IN): Integer-индекс новой текущей закладки
	// Returns:
	//   В случае, если параметр является допустимым индексом закладки, возвращает ее идентификатор (Tab),
	//   в противном случае возвращает Tab.tabUndef
	//
	public Tab OnTabSelection(Object subj /*must be Integer*/)
	{
		Tab result = Tab.tabUndef;
		if(subj != null && subj instanceof Integer) {
			int tab_idx = (Integer)subj;
			if(TabList != null && tab_idx >= 0 && tab_idx < TabList.size()) {
				TabEntry te = TabList.get(tab_idx);
				if(te != null) {
					result = te.TabId;
					final boolean was_empty = TabNavStack.empty();
					if(!was_empty) {
						Tab last = TabNavStack.peek();
						if(last != te.TabId)
							TabNavStack.push(te.TabId);
					}
					else
						TabNavStack.push(te.TabId);
					if(Callback_BackButton != null) {
						Callback_BackButton.setEnabled(!was_empty);
					}
				}
			}
		}
		return result;
	}
	//
	// Descr: Обрабатывает команду "назад" в локальном контексте. А именно, переключает вкладку
	//   табулятора назад по стеку TabNavStack
	//
	public boolean BackTab(@IdRes int viewPagerRcId)
	{
		boolean result = false;
		if(!TabNavStack.empty()) {
			//
			// Некоторые страницы viewPager'а могут быть невидимыми к моменту нажатия back-button
			// Из-за этого мы должны исполнить цикл дабы найти первую видимую страницу в стеке TabNavStack
			//
			final Tab current_tab_id = TabNavStack.pop(); // Это - табулятор, на котором мы сейчас находимся. Просто удаляем его из стека.
			while(!result && !TabNavStack.empty()) {
				Tab tab_id = TabNavStack.pop(); // Это - табулятор, на котором бы были перед текущим табулятором. Он нам и нужен!
				if(IsTabVisible(tab_id)) {
					if(Callback_BackButton != null && TabNavStack.empty()) {
						Callback_BackButton.setEnabled(false);
					}
					Implement_GotoTab(tab_id, viewPagerRcId, 0, -1, -1, -1);
					result = true;
				}
			}
			if(!result) {
				TabNavStack.push(current_tab_id); // Если после извлечения текущего табулятора стек пустой, то
					// значит нам некуда возвращаться - заталкиваем текущий табулятор назад и уходим.
			}
		}
		return result;
	}
	public boolean IsLocalBackTabNeeded()
	{
		return (!TabNavStack.empty() && TabNavStack.size() > 1);
	}
	public void GotoSearchTab(@IdRes int viewPagerRcId, int objToSearch)
	{
		Implement_GotoTab(Tab.tabSearch, viewPagerRcId, 0, -1, -1, objToSearch);
	}
	public void Implement_GotoTab(CommonPrereqModule.Tab tab, @IdRes int viewPagerRcId, @IdRes int recyclerViewToUpdate, int goToIndex, int nestedIndex, int objToSearch)
	{
		if(ActivityInstance != null && tab != CommonPrereqModule.Tab.tabUndef) {
			ViewPager2 view_pager = (ViewPager2)ActivityInstance.findViewById(viewPagerRcId);
			if(view_pager != null) {
				for(int tidx = 0; tidx < TabList.size(); tidx++) {
					final CommonPrereqModule.TabEntry te = TabList.get(tidx);
					if(te.TabId == tab) {
						SLib.SlFragmentStatic f = te.TabView;
						if(f != null) {
							view_pager.setCurrentItem(tidx, false);
							//TabNavStack.push(tab); // @v11.5.0
							if(recyclerViewToUpdate != 0) {
								View fv2 = view_pager.getChildAt(tidx);
								//f.requireView();
								View fv = f.getView();
								if(fv != null && fv instanceof ViewGroup) {
									fv.refreshDrawableState(); // @v11.4.3
									View lv = fv.findViewById(recyclerViewToUpdate);
									if(lv != null && lv instanceof RecyclerView) {
										RecyclerView.Adapter gva = ((RecyclerView)lv).getAdapter();
										if(gva != null) {
											if(goToIndex >= 0 && goToIndex < gva.getItemCount()) {
												ActivityInstance.SetRecyclerListFocusedIndex(gva, goToIndex);
												((RecyclerView)lv).scrollToPosition(goToIndex);
											}
											gva.notifyDataSetChanged();
										}
									}
								}
							}
							if(objToSearch > 0 && tab == Tab.tabSearch) {
								SelectSearchPaneObjRestriction(f.getView(), objToSearch);
							}
						}
						break;
					}
				}
			}
		}
	}
	public void NotifyTabContentChanged(@IdRes int viewPagerRcId, CommonPrereqModule.Tab tab, int innerViewId)
	{
		if(ActivityInstance != null && tab != CommonPrereqModule.Tab.tabUndef) {
			View vp = ActivityInstance.findViewById(viewPagerRcId);
			if(vp != null && vp instanceof ViewPager2 && TabList != null) {
				for(int tidx = 0; tidx < TabList.size(); tidx++) {
					if(TabList.get(tidx).TabId == tab) {
						SLib.SlFragmentStatic f = TabList.get(tidx).TabView;
						if(f != null) {
							View fv = f.getView();
							View lv = (fv != null && innerViewId != 0) ? fv.findViewById(innerViewId) : null;
							if(lv != null) {
								if(lv instanceof RecyclerView) {
									RecyclerView.Adapter gva = ((RecyclerView) lv).getAdapter();
									if(gva != null)
										gva.notifyDataSetChanged();
								}
								else
									lv.refreshDrawableState();
							}
						}
						break;
					}
				}
			}
		}
	}
	public Tab GetCurrentTabId()
	{
		Tab result = Tab.tabUndef;
		if(ActivityInstance != null && ViewPagerResourceId != 0 && TabLayoutResourceId != 0) {
			ViewPager2 view_pager = (ViewPager2)ActivityInstance.findViewById(ViewPagerResourceId);
			if(view_pager != null && TabList != null) {
				int tab_idx = view_pager.getCurrentItem();
				if(tab_idx >= 0 && tab_idx < TabList.size()) {
					final TabEntry te = TabList.get(tab_idx);
					if(te != null)
						result = te.TabId;
				}
			}
		}
		return result;
	}
	public boolean IsTabVisible(Tab tabId)
	{
		boolean result = false;
		if(ActivityInstance != null && ViewPagerResourceId != 0 && TabLayoutResourceId != 0) {
			ViewPager2 view_pager = (ViewPager2)ActivityInstance.findViewById(ViewPagerResourceId);
			if(view_pager != null && TabList != null) {
				for(int tidx = 0; tidx < TabList.size(); tidx++) {
					if(TabList.get(tidx).TabId == tabId) {
						View v_lo_tab = ActivityInstance.findViewById(TabLayoutResourceId);
						if(v_lo_tab != null && v_lo_tab instanceof TabLayout) {
							TabLayout lo_tab = (TabLayout)v_lo_tab;
							int v = ((ViewGroup)lo_tab.getChildAt(0)).getChildAt(tidx).getVisibility();
							if(v == View.VISIBLE)
								result = true;
						}
						break;
					}
				}
			}
		}
		return result;
	}
	public void SetTabVisibility(CommonPrereqModule.Tab tabId, int visibilityMode)
	{
		if(ActivityInstance != null && ViewPagerResourceId != 0 && TabLayoutResourceId != 0) {
			ViewPager2 view_pager = (ViewPager2)ActivityInstance.findViewById(ViewPagerResourceId);
			if(view_pager != null && TabList != null) {
				for(int tidx = 0; tidx < TabList.size(); tidx++) {
					if(TabList.get(tidx).TabId == tabId) {
						View v_lo_tab = ActivityInstance.findViewById(TabLayoutResourceId);
						if(v_lo_tab != null && v_lo_tab instanceof TabLayout) {
							TabLayout lo_tab = (TabLayout)v_lo_tab;
							((ViewGroup)lo_tab.getChildAt(0)).getChildAt(tidx).setVisibility(visibilityMode);
						}
						break;
					}
				}
			}
		}
	}
	public void MakeCurrentDocList()
	{
		if(RegistryHList != null)
			RegistryHList.clear();
		try {
			if(SvcIdent != null) {
				StyloQApp app_ctx = GetAppCtx();
				StyloQDatabase db = (app_ctx != null) ? app_ctx.GetDB() : null;
				if(db != null) {
					StyloQDatabase.SecStoragePacket svc_pack = db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, SvcIdent);
					if(svc_pack != null) {
						long svc_id = svc_pack.Rec.ID;
						ArrayList<Long> doc_id_list = db.GetDocIdListByType(+1, StyloQDatabase.SecStoragePacket.doctypGeneric, svc_id, null);
						if(doc_id_list != null) {
							for(int i = 0; i < doc_id_list.size(); i++) {
								long local_doc_id = doc_id_list.get(i);
								StyloQDatabase.SecStoragePacket local_doc_pack = db.GetPeerEntry(local_doc_id);
								final int st = StyloQDatabase.SecTable.Rec.GetDocStatus(local_doc_pack.Rec.Flags);
								boolean do_skip = false;
								if(Rf != null && (Rf.Flags & RegistryFilt.fHideRejected) != 0) {
									if(st == StyloQDatabase.SecStoragePacket.styloqdocstCANCELLEDDRAFT || st == StyloQDatabase.SecStoragePacket.styloqdocstREJECTED ||
										st == StyloQDatabase.SecStoragePacket.styloqdocstCANCELLED) {
										do_skip = true;
									}
								}
								if(!do_skip) {
									JSONObject js_doc = (local_doc_pack != null) ? local_doc_pack.Pool.GetJsonObject(SecretTagPool.tagRawData) : null;
									if(js_doc != null) {
										Document local_doc = new Document();
										if(local_doc.FromJsonObj(js_doc)) {
											final SLib.LDATE dt = local_doc.GetNominalDate();
											if(Rf != null && !Rf.Period.IsZero()) {
												if(!Rf.Period.CheckDate(dt))
													do_skip = true;
											}
											if(!do_skip) {
												if(local_doc.H != null)
													local_doc.H.Flags = local_doc_pack.Rec.Flags;
												if(RegistryHList == null)
													RegistryHList = new ArrayList<Document.DisplayEntry>();
												// Эти операторы нужны на начальном этапе разработки поскольку
												// финализация пакета документа появилась не сразу {
												if(local_doc.GetNominalAmount() == 0.0)
													local_doc.H.Amount = local_doc.CalcNominalAmount();
												// }
												RegistryHList.add(new Document.DisplayEntry(local_doc));
												local_doc.H = null;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		} catch(StyloQException exn) {
			;
		}
	}
	public static class GoodsFilt {
		ArrayList <Integer> GroupIdList;
		ArrayList <Integer> BrandList;
	}
	public ProcessorEntry FindProcessorItemByID(int id)
	{
		ProcessorEntry result = null;
		if(ProcessorListData != null && id > 0) {
			for(int i = 0; result == null && i < ProcessorListData.size(); i++) {
				ProcessorEntry entry = ProcessorListData.get(i);
				if(entry != null && entry.JsItem != null) {
					final int iter_id = entry.JsItem.optInt("id", 0);
					if(iter_id == id)
						result = entry;
				}
			}
		}
		return result;
	}
	public int FindProcessorItemIndexByID(int id)
	{
		int result = -1;
		if(ProcessorListData != null && id > 0) {
			for(int i = 0; result < 0 && i < ProcessorListData.size(); i++) {
				ProcessorEntry entry = ProcessorListData.get(i);
				if(entry != null && entry.JsItem != null) {
					final int iter_id = entry.JsItem.optInt("id", 0);
					if(iter_id == id)
						result = i;
				}
			}
		}
		return result;
	}
	public boolean IsThereProcessorAssocWithGoods(int goodsID)
	{
		boolean result = false;
		try {
			if(goodsID > 0 && ProcessorListData != null) {
				for(int i = 0; !result && i < ProcessorListData.size(); i++) {
					ProcessorEntry entry = ProcessorListData.get(i);
					if(entry != null && entry.JsItem != null) {
						JSONArray js_goods_list = entry.JsItem.optJSONArray("goods_list");
						if(js_goods_list != null) {
							for(int j = 0; !result && j < js_goods_list.length(); j++) {
								JSONObject js_goods_item = js_goods_list.getJSONObject(j);
								if(js_goods_item != null) {
									final int iter_id = js_goods_item.optInt("id", 0);
									if(iter_id == goodsID)
										result = true;
								}
							}
						}
					}
				}
			}
		} catch(JSONException exn) {
			result = false;
		}
		return result;
	}
	public ArrayList <ProcessorEntry> GetProcessorListByGoods(int goodsID)
	{
		ArrayList <ProcessorEntry> result = null;
		try {
			if(goodsID > 0 && ProcessorListData != null) {
				for(int i = 0; i < ProcessorListData.size(); i++) {
					ProcessorEntry entry = ProcessorListData.get(i);
					if(entry != null && entry.JsItem != null) {
						JSONArray js_goods_list = entry.JsItem.optJSONArray("goods_list");
						if(js_goods_list != null) {
							boolean found = false;
							for(int j = 0; !found && j < js_goods_list.length(); j++) {
								JSONObject js_goods_item = js_goods_list.getJSONObject(j);
								if(js_goods_item != null) {
									int iter_id = js_goods_item.optInt("id", 0);
									if(iter_id == goodsID) {
										if(result == null)
											result = new ArrayList<ProcessorEntry>();
										result.add(entry);
										found = true;
									}
								}
							}
						}
					}
				}
			}
		} catch(JSONException exn) {
			result = null;
		}
		return result;
	}
	public double GetPriceForPrc(int prcID, int goodsID)
	{
		double result = 0.0;
		if(goodsID > 0) {
			try {
				if(prcID > 0 && ProcessorListData != null) {
					for(int i = 0; i < ProcessorListData.size(); i++) {
						ProcessorEntry entry = ProcessorListData.get(i);
						if(entry != null && entry.JsItem != null && entry.JsItem.optInt("id", 0) == prcID) {
							JSONArray js_goods_list = entry.JsItem.optJSONArray("goods_list");
							if(js_goods_list != null) {
								for(int j = 0; j < js_goods_list.length(); j++) {
									JSONObject js_goods_item = js_goods_list.getJSONObject(j);
									int iter_id = (js_goods_item != null) ? js_goods_item.optInt("id", 0) : 0;
									if(iter_id == goodsID) {
										result = js_goods_item.optDouble("price", 0.0);
										break; // В этом цикле такой же товар больше не встретится (если, конечно, нет ошибок в данных)
									}
								}
							}
							break;
						}
					}
				}
				if(result <= 0.0) {
					WareEntry ware_entry = FindGoodsItemByGoodsID(goodsID);
					if(ware_entry != null && ware_entry.Item != null)
						result = ware_entry.Item.Price;
				}
			} catch(JSONException exn) {
				;
			}
		}
		return result;
	}
	public int GetServiceDurationForPrc(int prcID, int goodsID)
	{
		int   result = 0;
		try {
			if(goodsID > 0 && ProcessorListData != null) {
				if(prcID > 0) {
					for(int i = 0; i < ProcessorListData.size(); i++) {
						ProcessorEntry entry = ProcessorListData.get(i);
						if(entry != null && entry.JsItem != null && entry.JsItem.optInt("id", 0) == prcID) {
							JSONArray js_goods_list = entry.JsItem.optJSONArray("goods_list");
							if(js_goods_list != null) {
								for(int j = 0; j < js_goods_list.length(); j++) {
									JSONObject js_goods_item = js_goods_list.getJSONObject(j);
									int iter_id = (js_goods_item != null) ? js_goods_item.optInt("id", 0) : 0;
									if(iter_id == goodsID) {
										result = js_goods_item.optInt("duration", 0);
										break; // В этом цикле такой же товар больше не встретится (если, конечно, нет ошибок в данных)
									}
								}
							}
							break;
						}
					}
				}
				else {
					int _sum = 0;
					int _count = 0;
					for(int i = 0; i < ProcessorListData.size(); i++) {
						ProcessorEntry entry = ProcessorListData.get(i);
						if(entry != null && entry.JsItem != null) {
							JSONArray js_goods_list = entry.JsItem.optJSONArray("goods_list");
							if(js_goods_list != null) {
								for(int j = 0; j < js_goods_list.length(); j++) {
									JSONObject js_goods_item = js_goods_list.getJSONObject(j);
									int iter_id = (js_goods_item != null) ? js_goods_item.optInt("id", 0) : 0;
									if(iter_id == goodsID) {
										int _duration = js_goods_item.optInt("duration", 0);
										if(_duration > 0) {
											_sum += _duration;
											_count++;
											break; // В этом цикле такой же товар больше не встретится (если, конечно, нет ошибок в данных)
										}
									}
								}
							}
						}
					}
					if(_sum > 0) {
						assert(_count > 0);
						result = (int)(((double)_sum) / ((double)_count));
					}
				}
			}
		} catch(JSONException exn) {
			result = 0;
		}
		return result;
	}
	public ArrayList <WareEntry> GetGoodsListByPrc(int prcID)
	{
		ArrayList <WareEntry> result = null;
		try {
			if(prcID > 0 && ProcessorListData != null && GoodsListData != null) {
				for(int i = 0; i < ProcessorListData.size(); i++) {
					ProcessorEntry entry = ProcessorListData.get(i);
					if(entry != null && entry.JsItem != null && entry.JsItem.optInt("id", 0) == prcID) {
						JSONArray js_goods_list = entry.JsItem.optJSONArray("goods_list");
						if(js_goods_list != null) {
							for(int j = 0; j < js_goods_list.length(); j++) {
								JSONObject js_goods_item = js_goods_list.getJSONObject(j);
								int iter_id = (js_goods_item != null) ? js_goods_item.optInt("id", 0) : 0;
								if(iter_id > 0) {
									WareEntry org_ware_entry = FindGoodsItemByGoodsID(iter_id);
									if(org_ware_entry != null) {
										int duration = (js_goods_item != null) ? js_goods_item.optInt("duration", 0) : 0;
										double price = (js_goods_item != null) ? js_goods_item.optDouble("price", 0.0) : 0.0;
										if(result == null)
											result = new ArrayList<WareEntry>();
										WareEntry new_ware_entry = new WareEntry(org_ware_entry.Item);
										new_ware_entry.PrcPrice = price;
										result.add(new_ware_entry);
									}
								}
							}
						}
						break;
					}
				}
			}
		} catch(JSONException exn) {
			result = null;
		}
		return result;
	}
	public void ResetGoodsFiter()
	{
		Gf = null;
	}
	public boolean SetGoodsFilterByBrand(int brandID)
	{
		boolean ok = true;
		if(brandID > 0) {
			if(Gf == null)
				Gf = new GoodsFilt();
			if(Gf.BrandList == null)
				Gf.BrandList = new ArrayList<Integer>();
			Gf.GroupIdList = null;
			Gf.BrandList.clear();
			Gf.BrandList.add(brandID);
		}
		else
			ok = false;
		return ok;
	}
	public boolean SetGoodsFilterByGroup(int groupID)
	{
		boolean ok = true;
		if(groupID > 0) {
			if(Gf == null)
				Gf = new GoodsFilt();
			if(Gf.GroupIdList == null)
				Gf.GroupIdList = new ArrayList<Integer>();
			Gf.BrandList = null;
			Gf.GroupIdList.clear();
			Gf.GroupIdList.add(groupID);
		}
		else
			ok = false;
		return ok;
	}
	public @NotNull String FormatCurrency(double val) { return SLib.FormatCurrency(val, CSVCP.BaseCurrencySymb); }
	public String FormatQtty(double val, int uomID, boolean emptyOnZero)
	{
		String result = null;
		BusinessEntity.Uom uom = null;
		if(emptyOnZero && val == 0.0)
			result = "";
		else {
			if(uomID > 0 && UomListData != null) {
				for(int i = 0; uom == null && i < UomListData.size(); i++) {
					BusinessEntity.Uom local_uom = UomListData.get(i);
					if(local_uom != null && local_uom.ID == uomID) {
						uom = local_uom;
					}
				}
			}
			if(uom != null && (uom.Flags & BusinessEntity.Uom.fIntVal) != 0)
				result = SLib.formatdouble(val, 0);
			else
				result = SLib.formatdouble(val, 3);
		}
		return result;
	}
	public void GetCommonJsonFactors(JSONObject jsHead) { CSVCP.FromJson(jsHead); }
	public void MakeGoodsListFromCommonJson(JSONObject jsHead) throws JSONException
	{
		JSONArray temp_array = jsHead.optJSONArray("goods_list");
		if(temp_array != null) {
			GoodsListData = new ArrayList<WareEntry>();
			for(int i = 0; i < temp_array.length(); i++) {
				Object temp_obj = temp_array.get(i);
				if(temp_obj != null && temp_obj instanceof JSONObject) {
					int goods_id = ((JSONObject) temp_obj).optInt("id", 0);
					WareEntry new_entry = new WareEntry((JSONObject)temp_obj);
					if(ProcessorListData != null && IsThereProcessorAssocWithGoods(goods_id))
						new_entry.PrcExpandStatus = 1;
					else
						new_entry.PrcExpandStatus = 0;
					GoodsListData.add(new_entry);
				}
			}
			Collections.sort(GoodsListData, new Comparator<WareEntry>() {
				@Override public int compare(WareEntry lh, WareEntry rh)
				{
					String ls = (lh != null && lh.Item != null) ? lh.Item.Name : "";
					String rs = (rh != null && rh.Item != null) ? rh.Item.Name : "";
					return ls.toLowerCase().compareTo(rs.toLowerCase());
				}
			});
		}
	}
	public void MakeProcessorListFromCommonJson(JSONObject jsHead) throws JSONException
	{
		//ProcessorListData = jsHead.optJSONArray("processor_list");
		JSONArray temp_array = (jsHead != null) ? jsHead.optJSONArray("processor_list") : null;
		if(temp_array != null) {
			ProcessorListData = new ArrayList<ProcessorEntry>();
			for(int i = 0; i < temp_array.length(); i++) {
				Object temp_obj = temp_array.get(i);
				if(temp_obj != null && temp_obj instanceof JSONObject) {
					JSONObject js_prc_item = (JSONObject)temp_obj;
					ProcessorEntry new_entry = new ProcessorEntry(js_prc_item);
					JSONArray js_goods_list = js_prc_item.getJSONArray("goods_list");
					if(js_goods_list != null && js_goods_list.length() > 0)
						new_entry.GoodsExpandStatus = 1;
					else
						new_entry.GoodsExpandStatus = 0;
					ProcessorListData.add(new_entry);
				}
			}
			Collections.sort(ProcessorListData, new Comparator<ProcessorEntry>() {
				@Override public int compare(ProcessorEntry lh, ProcessorEntry rh)
				{
					String ls = lh.JsItem.optString("nm", "");
					String rs = rh.JsItem.optString("nm", "");
					return ls.toLowerCase().compareTo(rs.toLowerCase());
				}
			});
		}
	}
	public void MakeUomListFromCommonJson(JSONObject jsHead) throws JSONException
	{
		JSONArray js_uom_list = (jsHead != null) ? jsHead.optJSONArray("uom_list") : null;
		if(js_uom_list != null && js_uom_list.length() > 0) {
			UomListData = new ArrayList<BusinessEntity.Uom>();
			for(int i = 0; i < js_uom_list.length(); i++) {
				final JSONObject js_uom = js_uom_list.optJSONObject(i);
				if(js_uom != null) {
					BusinessEntity.Uom uom = new BusinessEntity.Uom();
					if(uom.FromJsonObj(js_uom))
						UomListData.add(uom);
					else
						uom = null;
				}
			}
		}
		else
			UomListData = null;
	}
	public void MakeGoodsGroupListFromCommonJson(JSONObject jsHead) throws JSONException
	{
		JSONArray temp_array = (jsHead != null) ? jsHead.optJSONArray("goodsgroup_list") : null;
		if(temp_array != null) {
			GoodsGroupListData = new ArrayList<JSONObject>();
			for(int i = 0; i < temp_array.length(); i++) {
				Object temp_obj = temp_array.get(i);
				if(temp_obj != null && temp_obj instanceof JSONObject)
					GoodsGroupListData.add((JSONObject)temp_obj);
			}
			Collections.sort(GoodsGroupListData, new Comparator<JSONObject>() {
				@Override public int compare(JSONObject lh, JSONObject rh)
				{
					String ls = lh.optString("nm", "");
					String rs = rh.optString("nm", "");
					return ls.toLowerCase().compareTo(rs.toLowerCase());
				}
			});
		}
	}
	public void MakeBrandListFromCommonJson(JSONObject jsHead) throws JSONException
	{
		JSONArray temp_array = (jsHead != null) ? jsHead.optJSONArray("brand_list") : null;
		if(temp_array != null) {
			BrandListData = new ArrayList<BusinessEntity.Brand>();
			for(int i = 0; i < temp_array.length(); i++) {
				Object temp_obj = temp_array.get(i);
				if(temp_obj != null && temp_obj instanceof JSONObject) {
					BusinessEntity.Brand new_entry = new BusinessEntity.Brand();
					if(new_entry.FromJsonObj((JSONObject)temp_obj))
						BrandListData.add(new_entry);
				}
			}
			Collections.sort(BrandListData, new Comparator<BusinessEntity.Brand>() {
				@Override public int compare(BusinessEntity.Brand lh, BusinessEntity.Brand rh)
				{
					String ls = lh.Name;
					String rs = rh.Name;
					return ls.toLowerCase().compareTo(rs.toLowerCase());
				}
			});
		}
	}
	public void MakeClientListFromCommonJson(JSONObject jsHead) throws JSONException
	{
		JSONArray temp_array = jsHead.optJSONArray("client_list");
		if(temp_array != null) {
			CliListData = new ArrayList<CommonPrereqModule.CliEntry>();
			for(int i = 0; i < temp_array.length(); i++) {
				Object temp_obj = temp_array.get(i);
				if(temp_obj != null && temp_obj instanceof JSONObject)
					CliListData.add(new CommonPrereqModule.CliEntry((JSONObject) temp_obj));
			}
			Collections.sort(CliListData, new Comparator<CommonPrereqModule.CliEntry>() {
				@Override public int compare(CommonPrereqModule.CliEntry lh, CommonPrereqModule.CliEntry rh)
				{
					String ls = lh.JsItem.optString("nm", "");
					String rs = lh.JsItem.optString("nm", "");
					return ls.toLowerCase().compareTo(rs.toLowerCase());
				}
			});
		}
	}
	public void MakeIncomingDocFromCommonJson(JSONObject jsHead) throws JSONException
	{
		JSONArray js_uom_list = (jsHead != null) ? jsHead.optJSONArray("doc_list") : null;
		if(js_uom_list != null && js_uom_list.length() > 0) {
			IncomingDocListData = new ArrayList<Document>();
			for(int i = 0; i < js_uom_list.length(); i++) {
				final JSONObject js_uom = js_uom_list.optJSONObject(i);
				if(js_uom != null) {
					Document doc = new Document();
					if(doc.FromJsonObj(js_uom))
						IncomingDocListData.add(doc);
					else
						doc = null;
				}
			}
		}
		else
			IncomingDocListData = null;
	}
	public int FindGoodsGroupItemIndexByID(int id)
	{
		int result = -1;
		if(GoodsGroupListData != null && id > 0) {
			for(int i = 0; result < 0 && i < GoodsGroupListData.size(); i++) {
				final int iter_id = GoodsGroupListData.get(i).optInt("id", 0);
				if(iter_id == id)
					result = i;
			}
		}
		return result;
	}
	int FindBrandItemIndexByID(int id)
	{
		int result = -1;
		if(BrandListData != null && id > 0) {
			for(int i = 0; result < 0 && i < BrandListData.size(); i++) {
				BusinessEntity.Brand item = BrandListData.get(i);
				if(item != null && item.ID == id)
					result = i;
			}
		}
		return result;
	}
	public WareEntry FindGoodsItemByGoodsID(int goodsID)
	{
		WareEntry result = null;
		if(GoodsListData != null && goodsID > 0) {
			for(int i = 0; result == null && i < GoodsListData.size(); i++) {
				final int goods_id = GoodsListData.get(i).Item.ID;
				if(goods_id == goodsID)
					result = GoodsListData.get(i);
			}
		}
		return result;
	}
	public BusinessEntity.PreprocessBarcodeResult PreprocessBarcode(String code)
	{
		return BusinessEntity.PreprocessBarcode(code, CSVCP.BarcodeWeightPrefix, CSVCP.BarcodeCountPrefix);
	}
	public ArrayList <WareEntry> SearchGoodsItemsByBarcode(String code)
	{
		ArrayList <WareEntry> result = null;
		if(SLib.GetLen(code) > 0) {
			if(GoodsListData != null && GoodsListData.size() > 0) {
				STokenRecognizer tr = new STokenRecognizer();
				int tokn = 0;
				STokenRecognizer.TokenArray nta = tr.Run(code);
				String preprocessed_code = null;
				if(nta.Has(STokenRecognizer.SNTOK_EAN13) > 0.0f || nta.Has(STokenRecognizer.SNTOK_EAN8) > 0.0f ||
						nta.Has(STokenRecognizer.SNTOK_UPCA) > 0.0f || nta.Has(STokenRecognizer.SNTOK_UPCE) > 0.0f) {
					preprocessed_code = code;
				}
				else {
					GTIN gtin_chzn = GTIN.ParseChZnCode(code, 0);
					if(gtin_chzn != null && gtin_chzn.GetChZnParseResult() > 0) {
						preprocessed_code = gtin_chzn.GetToken(GTIN.fldGTIN14);
						if(SLib.GetLen(preprocessed_code) == 14)
							preprocessed_code = preprocessed_code.substring(1);
						else {
							// Это - не правильная марка, стало быть даже пытаться искать по коду не станем
							preprocessed_code = null;
						}
					}
					else
						preprocessed_code = code;
				}
				if(SLib.GetLen(preprocessed_code) > 0) {
					for(int i = 0; i < GoodsListData.size(); i++) {
						final WareEntry entry = GoodsListData.get(i);
						if(entry != null && entry.Item != null) {
							BusinessEntity.GoodsCode ce = entry.Item.SearchCode(preprocessed_code);
							if(ce != null) {
								if(result == null)
									result = new ArrayList<WareEntry>();
								result.add(entry);
							}
						}
					}
				}
			}
		}
		return result;
	}
	int FindGoodsItemIndexByID(int id)
	{
		int result = -1;
		if(GoodsListData != null && id > 0) {
			for(int i = 0; result < 0 && i < GoodsListData.size(); i++) {
				final int iter_id = GoodsListData.get(i).Item.ID;
				if(iter_id == id)
					result = i;
			}
		}
		return result;
	}
	public WareEntry GetGoodsListItemByIdx(int idx)
	{
		WareEntry result = null;
		if(GoodsListData != null && idx >= 0 && idx < GoodsListData.size()) {
			if(Gf != null) {
				int counter = 0;
				for(int i = 0; result == null && i < GoodsListData.size(); i++) {
					if(CheckGoodsListItemForFilt(GoodsListData.get(i))) {
						if(counter == idx)
							result = GoodsListData.get(i);
						else
							counter++;
					}
				}
			}
			else
				result = GoodsListData.get(idx);
		}
		return result;
	}
	public int GetGoodsListSize()
	{
		int result = 0;
		if(GoodsListData != null) {
			if(Gf != null) {
				for(int i = 0; i < GoodsListData.size(); i++) {
					if(CheckGoodsListItemForFilt(GoodsListData.get(i)))
						result++;
				}
			}
			else
				result = GoodsListData.size();
		}
		return result;
	}
	public boolean CheckGoodsListItemForFilt(WareEntry item)
	{
		boolean result = false;
		if(item == null || item.Item == null)
			result = false;
		else if(Gf == null)
			result =  true;
		else {
			result = true;
			if(Gf.GroupIdList != null && Gf.GroupIdList.size() > 0) {
				int parid = item.Item.ParentID;
				result = false;
				if(parid > 0) {
					for(int i = 0; !result && i < Gf.GroupIdList.size(); i++)
						if(Gf.GroupIdList.get(i) == parid)
							result = true;
				}
			}
			if(result && Gf.BrandList != null && Gf.BrandList.size() > 0) {
				int brand_id = item.Item.BrandID;
				result = false;
				if(brand_id > 0) {
					for(int i = 0; !result && i < Gf.BrandList.size(); i++)
						if(Gf.BrandList.get(i) == brand_id)
							result = true;
				}
			}
		}
		return result;
	}
	public JSONObject FindClientEntry(int cliID)
	{
		JSONObject result = null;
		if(CliListData != null && cliID > 0) {
			for(int i = 0; i < CliListData.size(); i++) {
				CommonPrereqModule.CliEntry ce = CliListData.get(i);
				if(ce != null && ce.JsItem != null) {
					int _id = ce.JsItem.optInt("id", 0);
					if(_id == cliID) {
						result = ce.JsItem;
						break;
					}
				}
			}
		}
		return result;
	}
	public JSONObject FindDlvrLocEntryInCliEntry(JSONObject cliJs, int dlvrLocID)
	{
		JSONObject result = null;
		try {
			if(CliListData != null && cliJs != null && dlvrLocID > 0) {
				JSONArray dvlrloc_list_js = cliJs.optJSONArray("dlvrloc_list");
				if(dvlrloc_list_js != null && dvlrloc_list_js.length() > 0) {
					for(int j = 0; j < dvlrloc_list_js.length(); j++) {
						JSONObject dlvrloc_js = dvlrloc_list_js.getJSONObject(j);
						if(dlvrloc_js != null) {
							int iter_id = dlvrloc_js.optInt("id", 0);
							if(iter_id == dlvrLocID) {
								result = dlvrloc_js;
								break;
							}
						}
					}
				}
			}
		} catch(JSONException exn) {
			result = null;
		}
		return result;
	}
	public int FindDlvrLocEntryIndexInCliEntry(JSONObject cliJs, int dlvrLocID)
	{
		int result = -1;
		try {
			if(CliListData != null && cliJs != null && dlvrLocID > 0) {
				JSONArray dvlrloc_list_js = cliJs.optJSONArray("dlvrloc_list");
				if(dvlrloc_list_js != null && dvlrloc_list_js.length() > 0) {
					for(int j = 0; j < dvlrloc_list_js.length(); j++) {
						JSONObject dlvrloc_js = dvlrloc_list_js.getJSONObject(j);
						if(dlvrloc_js != null) {
							int iter_id = dlvrloc_js.optInt("id", 0);
							if(iter_id == dlvrLocID) {
								result = j;
								break;
							}
						}
					}
				}
			}
		} catch(JSONException exn) {
			result = -1;
		}
		return result;
	}
	public JSONObject FindClientEntryByDlvrLocID(int dlvrLocID)
	{
		JSONObject result = null;
		if(CliListData != null && dlvrLocID > 0) {
			for(int i = 0; result == null && i < CliListData.size(); i++) {
				CommonPrereqModule.CliEntry ce = CliListData.get(i);
				if(ce != null && ce.JsItem != null) {
					if(FindDlvrLocEntryInCliEntry(ce.JsItem, dlvrLocID) != null)
						result = ce.JsItem;
				}
			}
		}
		return result;
	}
	private static class SearchDetailListAdapter extends SLib.InternalArrayAdapter {
		//private int RcId;
		SearchDetailListAdapter(Context ctx, int rcId, ArrayList data)
		{
			super(ctx, rcId, data);
			//RcId = rcId;
		}
		@Override public View getView(int position, View convertView, ViewGroup parent)
		{
			// Get the data item for this position
			Object item = (Object)getItem(position);
			//Context ctx = parent.getContext();
			Context _ctx = getContext();
			if(item != null && _ctx != null) {
				// Check if an existing view is being reused, otherwise inflate the view
				if(convertView == null) {
					convertView = LayoutInflater.from(_ctx).inflate(RcId, parent, false);
				}
				if(convertView != null) {
					if(item instanceof SimpleSearchBlock.IndexEntry) {
						SimpleSearchBlock.IndexEntry se = (SimpleSearchBlock.IndexEntry)item;
						String pattern = null;
						if(_ctx instanceof CmdROrderPrereqActivity)
							pattern = ((CmdROrderPrereqActivity)_ctx).CPM.GetSimpleSearchResultPattern();
						else if(_ctx instanceof CmdRAttendancePrereqActivity)
							pattern = ((CmdRAttendancePrereqActivity)_ctx).CPM.GetSimpleSearchResultPattern();
						{
							//SLib.SetCtrlString(convertView, R.id.CTL_SEARCHPANE_FOUNDTEXT, se.Text);
							TextView v = convertView.findViewById(R.id.CTL_SEARCHPANE_FOUNDTEXT);
							if(v != null) {
								final int text_len = SLib.GetLen(se.Text);
								final int pat_len = SLib.GetLen(pattern);
								final int fp_start = (pat_len > 0) ? se.Text.indexOf(pattern) : -1;
								if(fp_start >= 0) {
									int fp_end = fp_start+pat_len;
									SpannableStringBuilder spbldr = new SpannableStringBuilder();
									int color_reg = _ctx.getResources().getColor(R.color.ListItemRegular, _ctx.getTheme());
									int color_found = _ctx.getResources().getColor(R.color.ListItemFound, _ctx.getTheme());
									{
										SpannableString ss = new SpannableString(se.Text.substring(0, fp_start));
										ss.setSpan(new BackgroundColorSpan(color_reg), 0, ss.length(), 0);
										spbldr.append(ss);
									}
									{
										SpannableString ss = new SpannableString(se.Text.substring(fp_start, fp_end));
										ss.setSpan(new BackgroundColorSpan(color_found), 0, ss.length(), 0);
										spbldr.append(ss);
									}
									if(fp_end < (text_len-1)) {
										SpannableString ss = new SpannableString(se.Text.substring(fp_end, text_len-1));
										ss.setSpan(new BackgroundColorSpan(color_reg), 0, ss.length(), 0);
										spbldr.append(ss);
									}
									//v.setText(se.Text);
									v.setText(spbldr, TextView.BufferType.SPANNABLE);
								}
								else
									v.setText(se.Text);
							}
						}
						{
							TextView v = convertView.findViewById(R.id.CTL_SEARCHPANE_FOUNDDETAIL);
							if(v != null) {
								if(SLib.GetLen(se.DisplayText) > 0) {
									v.setVisibility(View.VISIBLE);
									v.setText(se.DisplayText);
								}
								else
									v.setVisibility(View.GONE);
							}
						}
					}
				}
			}
			return convertView; // Return the completed view to render on screen
		}
	}
	private void SetupSearchPaneObjRestrictionIcon(View fragmentView)
	{
		if(fragmentView != null) {
			View v = fragmentView.findViewById(R.id.CTLBUT_SEARCHPANE_OPTIONS);
			if(v != null && v instanceof ImageButton) {
				int rc_id = R.drawable.ic_asterisk01;
				switch(SsB.RestrictionObjType) {
					case SLib.PPOBJ_GOODS: rc_id = R.drawable.ic_obj_goods_kanji_054c1; break;
					case SLib.PPOBJ_GOODSGROUP: rc_id = R.drawable.ic_obj_goodsgroup; break;
					case SLib.PPOBJ_BRAND: rc_id = R.drawable.ic_obj_brand01; break;
					case SLib.PPOBJ_PERSON: rc_id = R.drawable.ic_client01; break;
					case SLib.PPOBJ_LOCATION: rc_id = R.drawable.ic_dlvrloc01; break;
					case SLib.PPOBJ_PROCESSOR: rc_id = R.drawable.ic_worker_stylist01; break;
					default: rc_id = R.drawable.ic_asterisk01; break;
				}
				((ImageButton)v).setImageResource(rc_id);
			}
		}
	}
	protected boolean OpenSearchPaneObjRestriction(View fragmentView)
	{
		boolean result = false;
		StyloQApp app_ctx = GetAppCtx();
		if(app_ctx != null && fragmentView != null) {
			if(SsB.ObjTypeList != null && SsB.ObjTypeList.size() > 0) {
				SLib.StrAssocArray obj_title_list = new SLib.StrAssocArray();
				obj_title_list.Set(0, "anything");
				for(Integer iter : SsB.ObjTypeList) {
					if(iter != null) {
						String obj_title = SLib.GetObjectTitle(app_ctx, iter);
						if(SLib.GetLen(obj_title) > 0) {
							obj_title_list.Set(iter, obj_title);
						}
					}
				}
				SLib.SetCtrlVisibility(fragmentView, R.id.CTLSEL_SEARCHPANE_OPTIONS, View.VISIBLE);
				SsB.RestrictionObjTypeCombonJustInited = true;
				SLib.SetupStrAssocCombo(app_ctx, fragmentView, R.id.CTLSEL_SEARCHPANE_OPTIONS, obj_title_list, SsB.RestrictionObjType);
			}
			else
				SLib.SetCtrlVisibility(fragmentView, R.id.CTLSEL_SEARCHPANE_OPTIONS, View.GONE);
			SetupSearchPaneObjRestrictionIcon(fragmentView);
		}
		return result;
	}
	protected boolean SelectSearchPaneObjRestriction(View fragmentView, int objType)
	{
		boolean result = false;
		if(SsB.RestrictionObjTypeCombonJustInited)
			SsB.RestrictionObjTypeCombonJustInited = false;
		else {
			if(objType >= 0) {
				SsB.RestrictionObjType = objType;
				if(fragmentView != null && fragmentView instanceof ViewGroup) {
					SLib.SetCtrlVisibility(fragmentView, R.id.CTLSEL_SEARCHPANE_OPTIONS, View.GONE);
					SearchPaneRunSearch(fragmentView);
				}
				result = true;
			}
		}
		SetupSearchPaneObjRestrictionIcon(fragmentView);
		return result;
	}
	private void SearchPaneRunSearch(View fragmentView)
	{
		if(ActivityInstance != null) {
			boolean sr = SearchInSimpleIndex(SsB.SearchPattern);
			String srit = SsB.SearchResult.GetSearchResultInfoText();
			if(!sr && SsB.SearchResult != null)
				SsB.SearchResult.Clear();
			SLib.SetCtrlString(fragmentView, R.id.CTL_SEARCHPANE_RESULTINFO, srit);
			View lv = ActivityInstance.findViewById(R.id.searchPaneListView);
			if(lv != null && lv instanceof RecyclerView) {
				RecyclerView.Adapter gva = ((RecyclerView) lv).getAdapter();
				if(gva != null)
					gva.notifyDataSetChanged();
			}
		}
	}
	protected void SetupSearchPaneListView(View fragmentView, View listView)
	{
		if(fragmentView != null && listView != null && listView instanceof RecyclerView) {
			((RecyclerView)listView).setLayoutManager(new LinearLayoutManager(ActivityInstance));
			ActivityInstance.SetupRecyclerListView(fragmentView, R.id.searchPaneListView, R.layout.li_searchpane_result);
			{
				SetupSearchPaneObjRestrictionIcon(fragmentView);
				View iv = fragmentView.findViewById(R.id.CTL_SEARCHPANE_INPUT);
				if(iv != null && iv instanceof TextInputEditText) {
					((TextInputEditText)iv).setText(SsB.SearchPattern);
					SLib.SetCtrlVisibility(fragmentView, R.id.CTLSEL_SEARCHPANE_OPTIONS, View.GONE);
					SelectSearchPaneObjRestriction(fragmentView, SsB.RestrictionObjType); // @v11.5.1
					TextInputEditText tiv = (TextInputEditText)iv;
					tiv.requestFocus();
					tiv.addTextChangedListener(new TextWatcher() {
						public void afterTextChanged(Editable s)
						{
							//int cross_icon_id = (s.length() > 0) ? R.drawable.ic_cross01 : 0;
							//tiv.setCompoundDrawablesWithIntrinsicBounds(0, 0, cross_icon_id, 0);
						}
						public void beforeTextChanged(CharSequence s, int start, int count, int after)
						{
						}
						public void onTextChanged(CharSequence s, int start, int before, int count)
						{
							SsB.SearchPattern = s.toString();
							SearchPaneRunSearch(fragmentView);
						}
					});
				}
			}
		}
	}
	public void GetSearchPaneListViewItem(View itemView, int itemIdx)
	{
		StyloQApp app_ctx = GetAppCtx();
		if(app_ctx != null && SsB.SearchResult != null && itemView != null && itemIdx < SsB.SearchResult.GetObjTypeCount()) {
			int obj_type = SsB.SearchResult.GetObjTypeByIndex(itemIdx);
			String obj_type_title = SLib.GetObjectTitle(app_ctx, obj_type);
			SLib.SetCtrlString(itemView, R.id.LVITEM_GENERICNAME, (obj_type_title != null) ? obj_type_title : "");
			{
				ListView detail_lv = (ListView)itemView.findViewById(R.id.searchPaneTerminalListView);
				ArrayList <SimpleSearchBlock.IndexEntry> detail_list = SsB.SearchResult.GetListByObjType(obj_type);
				if(detail_lv != null && detail_list != null) {
					SearchDetailListAdapter adapter = new SearchDetailListAdapter(/*this*/itemView.getContext(), R.layout.li_searchpane_resultdetail, detail_list);
					detail_lv.setAdapter(adapter);
					{
						int total_items_height = SLib.CalcListViewHeight(detail_lv);
						if(total_items_height > 0) {
							ViewGroup.LayoutParams params = detail_lv.getLayoutParams();
							params.height = total_items_height;
							detail_lv.setLayoutParams(params);
							detail_lv.requestLayout();
						}
					}
					adapter.setNotifyOnChange(true);
					detail_lv.setOnItemClickListener(new AdapterView.OnItemClickListener() {
						@Override public void onItemClick(AdapterView<?> parent, View view, int position, long id)
						{
							Object item = (Object)parent.getItemAtPosition(position);
							Context ctx = parent.getContext();
							if(item != null && ctx != null && ctx instanceof SLib.SlActivity) {
								SLib.SlActivity activity = (SLib.SlActivity)parent.getContext();
								SLib.ListViewEvent ev_subj = new SLib.ListViewEvent();
								ev_subj.ItemIdx = position;
								ev_subj.ItemId = id;
								ev_subj.ItemObj = item;
								ev_subj.ItemView = view;
								//ev_subj.ParentView = parent;
								activity.HandleEvent(SLib.EV_LISTVIEWITEMCLK, parent, ev_subj);
							}
						}
					});
				}
			}
		}
	}
	//
	//
	//
	static class RegistryFiltDialog extends SLib.SlDialog {
		RegistryFiltDialog(Context ctx, Object data)
		{
			super(ctx, R.id.DLG_REGISTRYFILT, data);
			if(data instanceof RegistryFilt)
				Data = data;
		}
		@Override public Object HandleEvent(int ev, Object srcObj, Object subj)
		{
			Object result = null;
			switch(ev) {
				case SLib.EV_CREATE:
					requestWindowFeature(Window.FEATURE_NO_TITLE);
					setContentView(R.layout.dialog_registry_filt);
					SetDTS(Data);
					break;
				case SLib.EV_CBSELECTED:
					if(subj != null && subj instanceof SLib.ListViewEvent) {
						SLib.ListViewEvent lve = (SLib.ListViewEvent)subj;
						if(lve.ItemIdx >= 0 && lve.ItemId >= 0) {
							if(srcObj != null) {
								RegistryFilt _data = GetDataInstance();
								if(_data.Period == null)
									_data.Period = new SLib.DateRange();
								_data.PredefPeriod = (int)lve.ItemId;
								_data.Period.SetPredefined(_data.PredefPeriod, null);
								SetupPeriod();
							}
						}
					}
					break;
				case SLib.EV_COMMAND:
					int view_id = View.class.isInstance(srcObj) ? ((View)srcObj).getId() : 0;
					if(view_id == R.id.STDCTL_OKBUTTON || view_id == R.id.STDCTL_CLOSEBUTTON) {
						Object data = GetDTS();
						if(data != null) {
							Context ctx = getContext();
							StyloQApp app_ctx = (StyloQApp)ctx.getApplicationContext();
							if(app_ctx != null)
								app_ctx.HandleEvent(SLib.EV_IADATAEDITCOMMIT, this, data);
						}
						this.dismiss(); // Close Dialog
					}
					else if(view_id == R.id.STDCTL_CANCELBUTTON) {
						this.dismiss(); // Close Dialog
					}
					else if(view_id == R.id.tbButtonPeriod) {
						if(Data != null && Data instanceof StyloQDatabase.SecStoragePacket) {
							Context ctx = getContext();
							StyloQApp app_ctx = (ctx != null) ? (StyloQApp)ctx.getApplicationContext() : null;
							if(app_ctx != null) {
								/*
								try {
									StyloQDatabase db = app_ctx.GetDB();
									db.DeleteForeignSvc(((StyloQDatabase.SecStoragePacket)Data).Rec.ID);
									app_ctx.HandleEvent(SLib.EV_IADATADELETECOMMIT, this, Data);
									this.dismiss(); // Close Dialog
								} catch(StyloQException exn) {
									;
								}
								 */
							}
						}
					}
					/*else if(view_id == R.id.STDCTL_CLOSEBUTTON) {
						this.dismiss();
					}*/
					break;
			}
			return result;
		}
		private RegistryFilt GetDataInstance()
		{
			RegistryFilt _data = null;
			if(Data != null && Data.getClass().getSimpleName().equals("RegistryFilt"))
				_data = (RegistryFilt)Data;
			else {
				_data = new RegistryFilt();
				Data = _data;
			}
			return _data;
		}
		private void SetupPeriod()
		{
			RegistryFilt _data = GetDataInstance();
			String period_text = null;
			if(_data.Period != null)
				period_text = _data.Period.Format();
			SLib.SetCtrlString(this, R.id.CTL_REGISTRYFILT_PERIOD, period_text);
		}
		boolean SetDTS(Object objData)
		{
			boolean ok = true;
			Context ctx = getContext();
			StyloQApp app_ctx = (ctx != null) ? (StyloQApp)ctx.getApplicationContext() : null;
			if(app_ctx != null) {
				RegistryFilt _data = GetDataInstance();
				{
					// 1 - сегодня, 2 - вчера, 3 - текущая неделя, 4 - предыдущая неделя, 5 - текущий месяц, 6 - предыдущий месяц, 1000 - picker
					SLib.StrAssocArray selection_list = new SLib.StrAssocArray();
					{
						selection_list.Set(SLib.PREDEFPRD_NONE, "");
						selection_list.Set(SLib.PREDEFPRD_TODAY, app_ctx.GetString("today"));
						selection_list.Set(SLib.PREDEFPRD_YESTERDAY, app_ctx.GetString("yesterday"));
						selection_list.Set(SLib.PREDEFPRD_THISWEEK, app_ctx.GetString("currentweek"));
						selection_list.Set(SLib.PREDEFPRD_LASTWEEK, app_ctx.GetString("lastweek"));
						selection_list.Set(SLib.PREDEFPRD_THISMONTH, app_ctx.GetString("currentmonth"));
						selection_list.Set(SLib.PREDEFPRD_LASTMONTH, app_ctx.GetString("lastmonth"));
					}
					SLib.SetupStrAssocCombo(app_ctx, this, R.id.CTLSEL_REGISTRYFILT_MACROPERIOD, selection_list, (_data != null) ? _data.PredefPeriod : SLib.PREDEFPRD_NONE);
				}
				SetupPeriod();
				SLib.SetCheckboxState(this, R.id.CTL_REGISTRYFILT_FLAG01, (_data.Flags & RegistryFilt.fHideRejected) != 0);
			}
			return ok;
		}
		Object GetDTS()
		{
			Object result = null;
			RegistryFilt _data = null;
			if(Data != null && Data.getClass().getSimpleName().equals("RegistryFilt"))
				_data = (RegistryFilt)Data;
			else {
				_data = new RegistryFilt();
				Data = _data;
			}
			String period_text = SLib.GetCtrlString(this, R.id.CTL_REGISTRYFILT_PERIOD);
			if(_data.Period == null)
				_data.Period = new SLib.DateRange();
			_data.Period.FromString(period_text);
			if(SLib.GetCheckboxState(this, R.id.CTL_REGISTRYFILT_FLAG01))
				_data.Flags |= RegistryFilt.fHideRejected;
			else
				_data.Flags &= ~RegistryFilt.fHideRejected;
			result = _data;
			return result;
		}
	}
}
