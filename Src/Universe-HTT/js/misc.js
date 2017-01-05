
function checkINN(inn) {
	if(!isEmpty(inn)) {
		if(!isNaN(inn)) {
			var n1, n2;
			var d = inn.split("");
			var lenght = inn.length;
			if(lenght == 10) {
				n1 = (d[0]*2 + d[1]*4 + d[2]*10 + d[3]*3 + d[4]*5 + d[5]*9 + d[6]*4 + d[7]*6 + d[8]*8) % 11;
				if(n1 == d[9])
					return true;
			}
			else if(lenght == 12) {
				n2 = (d[0]*7 + d[1]*2 + d[2]*4 + d[3]*10 + d[4]*3 + d[5]*5 + d[6]*9 + d[7]*4 + d[8]*6 + d[9]*8) % 11;
				n1 = (d[0]*3 + d[1]*7 + d[2]*2 + d[3]*4 + d[4]*10 + d[5]*3 + d[6]*5 + d[7]*9 + d[8]*4 + d[9]*6 + d[10]*8) % 11;
				if(n2 == d[10] && n1 == d[11])
					return true;
			}
		}
	}
	return false;
};

function checkBarcode(code) {
	if(!isEmpty(code)) {
		code = code.trim();
		if(!isNaN(code)) {
			var n;
			var d = code.split("");
			var length = code.length;
			switch(length) {
				case 8: /* EAN-8 */ 
					n = d[0]*3 + d[1]*1 + d[2]*3 + d[3]*1 + d[4]*3 + d[5]*1 +d[6]*3 + d[7]*1;
					if(n%10 == 0)
						return true;
					break;
				case 12: /* UPC-12 */
					n = d[0]*3 + d[1]*1 + d[2]*3 + d[3]*1 + d[4]*3 + d[5]*1 +d[6]*3 + d[7]*1 + d[8]*3 + d[9]*1 + d[10]*3 + d[11]*1;
					if(n%10 == 0)
						return true;
					break;
				case 13: /* EAN-13 */
					n = d[0]*1 + d[1]*3 + d[2]*1 + d[3]*3 + d[4]*1 + d[5]*3 +d[6]*1 + d[7]*3 + d[8]*1 + d[9]*3 + d[10]*1 + d[11]*3 + d[12]*1;
					if(n%10 == 0)
						return true;
					break;
			}
		}
	}
	return false;
};

function showGoodsDetails(goods_id, title, in_browser) {
	var title_r = "Информация о товаре";
	if(!isEmpty(title))
		title_r = title;
	if(in_browser == true)
		UHTT.Browser.show(
				UHTT.Content.get("GOODS_DETAILS", {ID: goods_id}),
				title_r);
	else
		new SDialog(
				"UHTTGoodsInfoDlg",
				UHTT.Content.get("GOODS_DETAILS", {ID: goods_id}),
				{ resizable: false, title: title_r },
				UHTT.Dialog.TYPE.INLINE);
};

function showBrandDetails(brand_id, title) {
	var query = "GETTDDO FRM_BRAND_DETAILS " + brand_id;
	var title_r = "Информация о бренде";
	if(!isEmpty(title) && title != "undefined")
		title_r = title;
	new SDialog("UHTTBrandInfoDlg", query, { resizable: false, width: 350, title: title_r });
};

function showPersonDetails(pers_id, title) {
	var query = "GETTDDO FRM_PERSON_DETAILS " + pers_id;
	var title_r = "Информация о персоналии";
	if(!isEmpty(title) && title != "undefined")
		title_r = title;
	new SDialog("UHTTPersonInfoDlg", query, { resizable: false, title: title_r });
};

function showOfferDetails(sid, id) {
	if(isEmpty(sid))
		sid = UHTT.Store.Preferences.SID;
	if(isEmpty(sid))
		throw new Error("Error: Invalid SID");
	new SDialog(
			"UHTT_OFFER_DETAILS_FRM",
			UHTT.Content.get("OFFER_DETAILS", {SID: sid, GoodsID: id}),
			{ title: "Информация о товаре", modal: true, position: "center" },
			UHTT.Dialog.TYPE.INLINE);
};
