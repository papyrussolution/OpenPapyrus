package ru.petroglif.styloq;

import android.content.Context;
import android.text.TextPaint;
import android.view.ContextThemeWrapper;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.text.DecimalFormat;
import java.util.ArrayList;

public class ViewDescriptionList {
	public static class Item {
		static final int fAllNumeric = 0x0001;
		static final int fImage      = 0x0002;
		Item()
		{
			Id = 0; // @v11.4.3
			TotalFunc = 0;
			RTotalResult = 0.0;
			ITotalResult = 0;
			FixedWidth = 0;
			FixedHeight = 0;
			LayoutWeight = 0.0f;
			LayoutWidth = 0.0f;
			//AllNumeric = false;
			Flags = 0;
			ForceAlignment = 0;
			Mrgn = null;
			StyleRcId = 0;
			DataTypeBTS = 0;
			SlFormat = 0;
			SlFormat2 = 0;
		}
		int    Id; // @v11.4.3 optional anchor id
		String Zone;
		String FieldName;
		String Title;
		int    DataTypeBTS; // @v11.4.1 BTS_XXX (отображается в json как "FieldType"
		int    SlFormat;    // @v11.4.2 Параметры форматирования, передаваемые сервисом
		int    SlFormat2;   // @v11.4.2 Параметры форматирования, передаваемые сервисом
		int    StyleRcId;
		int    ForceAlignment; // >0 - left, <0 - right
		int    TotalFunc;
		double RTotalResult; // @v11.3.1
		int    ITotalResult; // @v11.3.1
		int    Flags;
		int    FixedWidth;  // dp (<=0 - undefined)
		int    FixedHeight; // dp (<=0 - undefined)
		float  LayoutWeight;
		float  LayoutWidth; // @v11.2.11
		SLib.Margin Mrgn;  // @v11.3.12
	}
	public static class DataPreprocessBlock {
		DataPreprocessBlock(Item columnDescription)
		{
			ColumnDescription = columnDescription;
			NzTextCount = 0;
			TextLenSum = 0.0f;
			TextLenMax = 0.0f;
			Count = 0;
			Sum = 0.0;
			Max = -Double.MAX_VALUE;
			Min = Double.MAX_VALUE;
			V = null;
			Tp = null;
		}
		Item   ColumnDescription;
		int    NzTextCount; // Количество элементов данных с непустым текстом
		float  TextLenSum;
		float  TextLenMax;
		int    Count;
		double Sum;
		double Max;
		double Min;
		View   V;// = Vdl.CreateBaseEntryTextView(this, 0, 0, i);
		TextPaint Tp;// = tv.getPaint();
	}
	private ArrayList<Item> L;
	private int HeaderStyleRcId;
	private int BottomStyleRcId;
	private int ItemStyleRcId;
	//
	public ViewDescriptionList()
	{
		L = null;
		HeaderStyleRcId = 0;
		BottomStyleRcId = 0;
		ItemStyleRcId = 0;
	}
	public void SetHeaderStyleRcId(int rcId) { HeaderStyleRcId = rcId; }
	public void SetBottomStyleRcId(int rcId) { BottomStyleRcId = rcId; }
	public void SetItemStyleRcId(int rcId) { ItemStyleRcId = rcId; }
	boolean IsThereTotals()
	{
		boolean result = false;
		if(L != null && L.size() > 0) {
			for(int i = 0; !result && i < L.size(); i++) {
				if(L.get(i).TotalFunc > 0)
					result = true;
			}
		}
		return result;
	}
	boolean AddItem(Item i)
	{
		boolean ok = true;
		if(i != null) {
			if(L == null)
				L = new ArrayList<Item>();
			L.add(i);
		}
		else
			ok = false;
		return ok;
	}
	int  GetCount()
	{
		return (L != null) ? L.size() : 0;
	}
	Item Get(int idx)
	{
		return (L != null && idx >= 0 && idx < L.size()) ? L.get(idx) : null;
	}
	boolean FromJsonObj(JSONObject jsObj) throws StyloQException
	{
		boolean result = false;
		if(jsObj != null) {
			try {
				JSONArray _vdl = jsObj.optJSONArray("Items");
				if(_vdl != null) {
					L = new ArrayList<Item>();
					int i;
					for(i = 0; i < _vdl.length(); i++) {
						JSONObject _vdl_item = (JSONObject)_vdl.get(i);
						if(_vdl_item != null) {
							Item new_item = new Item();
							new_item.Zone = _vdl_item.optString("Zone", "");
							new_item.FieldName = _vdl_item.optString("FieldName", "");
							new_item.Title = _vdl_item.optString("Text", "");
							new_item.TotalFunc = _vdl_item.optInt("TotalFunc", 0);
							String field_type_text = _vdl_item.optString("FieldType", null);
							if(SLib.GetLen(field_type_text) > 0) {
								if(field_type_text.equalsIgnoreCase("int"))
									new_item.DataTypeBTS = DataType.BTS_INT;
								else if(field_type_text.equalsIgnoreCase("real"))
									new_item.DataTypeBTS = DataType.BTS_REAL;
								else if(field_type_text.equalsIgnoreCase("string"))
									new_item.DataTypeBTS = DataType.BTS_STRING;
								else if(field_type_text.equalsIgnoreCase("date"))
									new_item.DataTypeBTS = DataType.BTS_DATE;
								else if(field_type_text.equalsIgnoreCase("time"))
									new_item.DataTypeBTS = DataType.BTS_TIME;
								else if(field_type_text.equalsIgnoreCase("timestamp"))
									new_item.DataTypeBTS = DataType.BTS_DATETIME;
								else
									new_item.DataTypeBTS = 0;
							}
							else
								new_item.DataTypeBTS = 0;
							new_item.SlFormat = _vdl_item.optInt("SlFormat", 0);
							new_item.SlFormat2 = _vdl_item.optInt("SlFormat2", 0);
							AddItem(new_item);
							result = true;
						}
					}
				}
			} catch(JSONException exn) {
				result = false;
				throw new StyloQException(ppstr2.PPERR_JEXN_JSON, exn.getMessage());
			}
		}
		return result;
	}
	//
	// Descr: Высокоуровневая функция, создающая и форматирующая экземпляр TextView
	//   на основании параметров элемента с индексом columnIdx.
	// ARG(phase IN): 0 - preprocess, 1 - running
	// ARG(level IN): 0 - detail, 1 - header, 2 - footer
	//
	public View CreateBaseEntryView(Context ctx, int phase, int level, int columnIdx/*0..*/)
	{
		View result = null;
		if(L != null && columnIdx >= 0 && columnIdx < L.size()) {
			Item di = L.get(columnIdx);
			TextView tv = null;
			ImageView iv = null;
			if(level == 0 && (di.Flags & Item.fImage) != 0) {
				if(di.StyleRcId != 0)
					iv = new ImageView(new ContextThemeWrapper(ctx, di.StyleRcId));
				else
					iv = new ImageView(ctx);
				result = iv;
			}
			else {
				if(di.StyleRcId != 0)
					tv = new TextView(new ContextThemeWrapper(ctx, di.StyleRcId));
				else
					tv = new TextView(ctx);
				tv.setSingleLine();
				int alignment = View.TEXT_ALIGNMENT_TEXT_START;
				if(phase == 1 && level == 0) {
					if(di.ForceAlignment > 0)
						alignment = View.TEXT_ALIGNMENT_TEXT_START;
					else if(di.ForceAlignment < 0)
						alignment = View.TEXT_ALIGNMENT_TEXT_END;
					else if(di.DataTypeBTS == DataType.BTS_INT || di.DataTypeBTS == DataType.BTS_REAL)
						alignment = View.TEXT_ALIGNMENT_TEXT_END;
					else if((di.Flags & Item.fAllNumeric) != 0)
						alignment = View.TEXT_ALIGNMENT_TEXT_END;
				}
				tv.setTextAlignment(alignment);
				result = tv;
			}
			/*
			if(phase == 1)
				result.setAutoSizeTextTypeWithDefaults(TextView.AUTO_SIZE_TEXT_TYPE_UNIFORM);
			 */
			if(level == 0) {
				result.setId(columnIdx + 1);
			}
			else if(tv != null) {
				if(level == 1) {
					tv.setText(di.Title);
				}
				else if(level == 2) {
					if(di.TotalFunc > 0)
						if(di.TotalFunc == SLib.AGGRFUNC_COUNT)
							tv.setText(Integer.toString(di.ITotalResult));
						else {
							int prec = 0;
							if(di.SlFormat != 0)
								prec = SLib.SFMTPRC(di.SlFormat);
							else
								prec = 2;
							DecimalFormat df = new DecimalFormat();
							df.setMaximumFractionDigits(prec);
							df.setMinimumFractionDigits(prec);
							String total_text = df.format(di.RTotalResult);
							tv.setText(total_text);
						}
					else
						tv.setText("");
				}
			}
			int lo_width = 0;
			float lo_weight = 0.0f/*di.LayoutWeight*/;
			if(phase == 0)
				lo_width = 0;
			else
				lo_width = (int)di.LayoutWidth;
			LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(/*LinearLayout.LayoutParams.WRAP_CONTENT*/lo_width,
				LinearLayout.LayoutParams.MATCH_PARENT, lo_weight);
			if(di.Mrgn != null)
				lp.setMargins(di.Mrgn.Left, di.Mrgn.Top, di.Mrgn.Right, di.Mrgn.Bottom);
			/*else
				lp.setMargins(6, 2, 6, 2);*/
			result.setLayoutParams(lp);
		}
		return result;
	}
	//
	// Descr: Высокоуровневая функция, создающая layout на основании собственного списка.
	//   Функция статическая так как должна создать layout даже в том случае, когда this == 0.
	// ARG(level IN): 0 - detail, 1 - header, 2 - footer
	//
	public static LinearLayout CreateItemLayout(ViewDescriptionList self, Context ctx, int level)
	{
		LinearLayout result = null;
		if(self != null) {
			if(level == 0 && self.ItemStyleRcId != 0)
				result = new LinearLayout(new ContextThemeWrapper(ctx, self.ItemStyleRcId));
			else if(level == 1 && self.HeaderStyleRcId != 0)
				result = new LinearLayout(new ContextThemeWrapper(ctx, self.HeaderStyleRcId));
			else if(level == 2 && self.BottomStyleRcId != 0)
				result = new LinearLayout(new ContextThemeWrapper(ctx, self.BottomStyleRcId));
		}
		if(result == null)
			result = new LinearLayout(ctx);
		result.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
		result.setOrientation(LinearLayout.HORIZONTAL);
		if(self != null) {
			final int cc = self.GetCount();
			for(int i = 0; i < cc; i++) {
				Item di = self.L.get(i);
				if(di != null) {
					View view = self.CreateBaseEntryView(ctx, 1, level, i);
					if(view != null)
						result.addView(view);
				}
			}
		}
		return result;
	}
	DataPreprocessBlock StartDataPreprocessing(Context ctx, int columnIdx)
	{
		DataPreprocessBlock result = null;
		if(L != null && columnIdx >= 0 && columnIdx < L.size()) {
			Item _item = L.get(columnIdx);
			result = new DataPreprocessBlock(_item);
			result.V = CreateBaseEntryView(ctx, 0, 0, columnIdx);
			if(result.V != null && result.V instanceof TextView) {
				result.Tp = ((TextView)result.V).getPaint();
			}
		}
		return result;
	}
	boolean DataPreprocessingIter(DataPreprocessBlock dpb, String text)
	{
		boolean ok = true;
		if(dpb != null && dpb.ColumnDescription != null) {
			dpb.Count++;
			if(SLib.GetLen(text) > 0) {
				float tw = dpb.Tp.measureText(text);
				if(tw > 0.0) {
					boolean is_num = false;
					if(dpb.ColumnDescription.DataTypeBTS == DataType.BTS_INT) {
						is_num = true;
					}
					else if(dpb.ColumnDescription.DataTypeBTS == DataType.BTS_REAL) {
						is_num = true;
					}
					else if(SLib.IsNumeric(text)) {
						is_num = true;
					}
					if(is_num) {
						double rv = Double.valueOf(text);
						dpb.Sum += rv;
						if(dpb.Max < rv)
							dpb.Max = rv;
						if(dpb.Min > rv)
							dpb.Min = rv;
					}
					else {
						dpb.ColumnDescription.Flags &= ~Item.fAllNumeric;
					}
					dpb.NzTextCount++;
					dpb.TextLenSum += tw;
					if(dpb.TextLenMax < tw)
						dpb.TextLenMax = tw;
				}
			}
		}
		else
			ok = false;
		return ok;
	}
	boolean DataPreprocessingIter(DataPreprocessBlock dpb, Object value, String text)
	{
		boolean ok = true;
		if(dpb != null && dpb.ColumnDescription != null) {
			dpb.Count++;
			if(SLib.GetLen(text) > 0) {
				float tw = dpb.Tp.measureText(text);
				if(tw > 0.0) {
					boolean is_number = false;
					double rv = 0.0;
					if(value != null && value instanceof Double) {
						rv = (Double)value;
						is_number = true;
					}
					else if(value != null && value instanceof Integer) {
						rv = (Double)value;
						is_number = true;
					}
					else if(value != null && value instanceof Long) {
						rv = (Double)value;
						is_number = true;
					}
					else if(SLib.IsNumeric(text)) {
						rv = Double.valueOf(text);
						is_number = true;
					}
					else {
						dpb.ColumnDescription.Flags &= ~Item.fAllNumeric;
					}
					if(is_number) {
						dpb.Sum += rv;
						if(dpb.Max < rv)
							dpb.Max = rv;
						if(dpb.Min > rv)
							dpb.Min = rv;
					}
					dpb.NzTextCount++;
					dpb.TextLenSum += tw;
					if(dpb.TextLenMax < tw)
						dpb.TextLenMax = tw;
				}
			}
		}
		else
			ok = false;
		return ok;
	}
	boolean FinishDataProcessing(DataPreprocessBlock dpb)
	{
		boolean ok = true;
		if(dpb != null && dpb.ColumnDescription != null) {
			if(dpb.ColumnDescription.TotalFunc > 0) {
				String total_text = null;
				boolean is_total_real = false;
				switch(dpb.ColumnDescription.TotalFunc) {
					case SLib.AGGRFUNC_COUNT:
						dpb.ColumnDescription.ITotalResult = dpb.Count;
						total_text = Integer.toString(dpb.Count);
						break;
					case SLib.AGGRFUNC_SUM:
						{
							dpb.ColumnDescription.RTotalResult = dpb.Sum;
							is_total_real = true;
							//total_text = Double.toString(dpb.ColumnDescription.RTotalResult);
							//StringBuilder sb = new StringBuilder();
							//Formatter f = new Formatter(sb, Locale.getDefault());
							//total_text = String.format("%10.2f", dpb.ColumnDescription.RTotalResult);
						}
						break;
					case SLib.AGGRFUNC_AVG:
						dpb.ColumnDescription.RTotalResult = (dpb.Count > 0) ? dpb.Sum / dpb.Count : 0.0;
						is_total_real = true;
						//total_text = Double.toString(dpb.ColumnDescription.RTotalResult);
						break;
					case SLib.AGGRFUNC_MAX:
						dpb.ColumnDescription.RTotalResult = (dpb.Max > -Double.MAX_VALUE) ? dpb.Max : 0.0;
						is_total_real = true;
						//total_text = Double.toString(dpb.ColumnDescription.RTotalResult);
						break;
					case SLib.AGGRFUNC_MIN:
						dpb.ColumnDescription.RTotalResult = (dpb.Min < Double.MAX_VALUE) ? dpb.Min : 0.0;
						is_total_real = true;
						//total_text = Double.toString(dpb.ColumnDescription.RTotalResult);
						break;
				}
				if(is_total_real) {
					int prec = 0;
					if(dpb.ColumnDescription.SlFormat != 0)
						prec = SLib.SFMTPRC(dpb.ColumnDescription.SlFormat);
					else
						prec = 2;
					DecimalFormat df = new DecimalFormat();
					df.setMaximumFractionDigits(prec);
					df.setMinimumFractionDigits(prec);
					total_text = df.format(dpb.ColumnDescription.RTotalResult);
				}
				if(SLib.GetLen(total_text) > 0) {
					float tw = dpb.Tp.measureText(total_text);
					if(tw > 0.0) {
						dpb.NzTextCount++;
						dpb.TextLenSum += tw;
						if(dpb.TextLenMax < tw)
							dpb.TextLenMax = tw;
					}
				}
			}
			{
				double avgl;
				if(dpb.NzTextCount > 0)
					avgl = (double)dpb.TextLenSum / (double)dpb.NzTextCount;
				else
					avgl = 1.0;
				if(dpb.ColumnDescription != null) {
					if(dpb.ColumnDescription.FixedWidth > 0) {
						dpb.ColumnDescription.LayoutWeight = 0.0f;
						dpb.ColumnDescription.LayoutWidth = dpb.ColumnDescription.FixedWidth;
					}
					else {
						dpb.ColumnDescription.LayoutWeight = (float)avgl;
						dpb.ColumnDescription.LayoutWidth = dpb.TextLenMax;
					}
				}
			}
		}
		else
			ok = false;
		return ok;
	}
}
