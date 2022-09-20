// SLib.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Application;
import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.database.DataSetObserver;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.Adapter;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.SpinnerAdapter;
import android.widget.TextView;
import androidx.activity.result.ActivityResult;
import androidx.activity.result.ActivityResultCallback;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.RecyclerView;
import androidx.viewpager2.adapter.FragmentStateAdapter;
import androidx.viewpager2.widget.ViewPager2;
import com.bumptech.glide.Glide;
import com.bumptech.glide.load.resource.bitmap.RoundedCorners;
import com.bumptech.glide.request.RequestOptions;
import com.google.android.material.tabs.TabLayout;
import com.google.android.material.textfield.TextInputLayout;
import org.jetbrains.annotations.NotNull;
import java.io.IOException;
import java.io.InputStream;
import java.math.BigInteger;
import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Comparator;
import java.util.Currency;
import java.util.Locale;
import java.util.Random;
import java.util.TimeZone;
import java.util.Timer;
import java.util.UUID;
import java.util.Vector;

public class SLib {
	//
	// @persistent Language constants
	// Values equal to SLIB constants with same symbols (see papyrus/src/include/slib.h)
	//
	public static final int slangMeta    =   1; // Meta-language
	public static final int slangLA   	 =   2; // LATIN
	public static final int slangEN   	 =   3; // ENGLISH
	public static final int slangRU   	 =   4; // RUSSIAN
	public static final int slangDE   	 =   5; // GERMAN
	public static final int slangAA   	 =   6; // AFAR
	public static final int slangAB   	 =   7; // ABKHAZIAN
	public static final int slangACE  	 =   8; // ACHINESE
	public static final int slangACH  	 =   9; // ACOLI
	public static final int slangADA  	 =  10; // ADANGME
	public static final int slangADY  	 =  11; // ADYGHE
	public static final int slangAE   	 =  12; // AVESTAN
	public static final int slangAF   	 =  13; // AFRIKAANS
	public static final int slangAFA  	 =  14; // AFRO-ASIATIC LANGUAGE
	public static final int slangAFH  	 =  15; // AFRIHILI
	public static final int slangAGQ  	 =  16; // AGHEM
	public static final int slangAIN  	 =  17; // AINU
	public static final int slangAK   	 =  18; // AKAN
	public static final int slangAKK  	 =  19; // AKKADIAN
	public static final int slangALE  	 =  20; // ALEUT
	public static final int slangALG  	 =  21; // ALGONQUIAN LANGUAGE
	public static final int slangALT  	 =  22; // SOUTHERN ALTAI
	public static final int slangAM   	 =  23; // AMHARIC
	public static final int slangAN   	 =  24; // ARAGONESE
	public static final int slangANG  	 =  25; // OLD ENGLISH
	public static final int slangANP  	 =  26; // ANGIKA
	public static final int slangAPA  	 =  27; // APACHE LANGUAGE
	public static final int slangAR   	 =  28; // ARABIC
	public static final int slangARC  	 =  29; // ARAMAIC
	public static final int slangARN  	 =  30; // ARAUCANIAN
	public static final int slangARP  	 =  31; // ARAPAHO
	public static final int slangART  	 =  32; // ARTIFICIAL LANGUAGE
	public static final int slangARW  	 =  33; // ARAWAK
	public static final int slangAS   	 =  34; // ASSAMESE
	public static final int slangASA  	 =  35; // ASU
	public static final int slangAST  	 =  36; // ASTURIAN
	public static final int slangATH  	 =  37; // ATHAPASCAN LANGUAGE
	public static final int slangAUS  	 =  38; // AUSTRALIAN LANGUAGE
	public static final int slangAV   	 =  39; // AVARIC
	public static final int slangAWA  	 =  40; // AWADHI
	public static final int slangAY   	 =  41; // AYMARA
	public static final int slangAZ   	 =  42; // AZERBAIJANI
	public static final int slangBA   	 =  43; // BASHKIR
	public static final int slangBAD  	 =  44; // BANDA
	public static final int slangBAI  	 =  45; // BAMILEKE LANGUAGE
	public static final int slangBAL  	 =  46; // BALUCHI
	public static final int slangBAN  	 =  47; // BALINESE
	public static final int slangBAS  	 =  48; // BASAA
	public static final int slangBAT  	 =  49; // BALTIC LANGUAGE
	public static final int slangBE   	 =  50; // BELARUSIAN
	public static final int slangBEJ  	 =  51; // BEJA
	public static final int slangBEM  	 =  52; // BEMBA
	public static final int slangBER  	 =  53; // BERBER
	public static final int slangBEZ  	 =  54; // BENA
	public static final int slangBG   	 =  55; // BULGARIAN
	public static final int slangBH   	 =  56; // BIHARI
	public static final int slangBHO  	 =  57; // BHOJPURI
	public static final int slangBI   	 =  58; // BISLAMA
	public static final int slangBIK  	 =  59; // BIKOL
	public static final int slangBIN  	 =  60; // BINI
	public static final int slangBLA  	 =  61; // SIKSIKA
	public static final int slangBM   	 =  62; // BAMBARA
	public static final int slangBN   	 =  63; // BENGALI
	public static final int slangBNT  	 =  64; // BANTU
	public static final int slangBO   	 =  65; // TIBETAN
	public static final int slangBR   	 =  66; // BRETON
	public static final int slangBRA  	 =  67; // BRAJ
	public static final int slangBRX  	 =  68; // BODO
	public static final int slangBS   	 =  69; // BOSNIAN
	public static final int slangBTK  	 =  70; // BATAK
	public static final int slangBUA  	 =  71; // BURIAT
	public static final int slangBUG  	 =  72; // BUGINESE
	public static final int slangBYN  	 =  73; // BLIN
	public static final int slangCA   	 =  74; // CATALAN
	public static final int slangCAD  	 =  75; // CADDO
	public static final int slangCAI  	 =  76; // CENTRAL AMERICAN INDIAN LANGUAGE
	public static final int slangCAR  	 =  77; // CARIB
	public static final int slangCAU  	 =  78; // CAUCASIAN LANGUAGE
	public static final int slangCAY  	 =  79; // CAYUGA
	public static final int slangCCH  	 =  80; // ATSAM
	public static final int slangCE   	 =  81; // CHECHEN
	public static final int slangCEB  	 =  82; // CEBUANO
	public static final int slangCEL  	 =  83; // CELTIC LANGUAGE
	public static final int slangCGG  	 =  84; // CHIGA
	public static final int slangCH   	 =  85; // CHAMORRO
	public static final int slangCHB  	 =  86; // CHIBCHA
	public static final int slangCHG  	 =  87; // CHAGATAI
	public static final int slangCHK  	 =  88; // CHUUKESE
	public static final int slangCHM  	 =  89; // MARI
	public static final int slangCHN  	 =  90; // CHINOOK JARGON
	public static final int slangCHO  	 =  91; // CHOCTAW
	public static final int slangCHP  	 =  92; // CHIPEWYAN
	public static final int slangCHR  	 =  93; // CHEROKEE
	public static final int slangCHY  	 =  94; // CHEYENNE
	public static final int slangCMC  	 =  95; // CHAMIC LANGUAGE
	public static final int slangCO   	 =  96; // CORSICAN
	public static final int slangCOP  	 =  97; // COPTIC
	public static final int slangCPE  	 =  98; // ENGLISH-BASED CREOLE OR PIDGIN
	public static final int slangCPF  	 =  99; // FRENCH-BASED CREOLE OR PIDGIN
	public static final int slangCPP  	 = 100; // PORTUGUESE-BASED CREOLE OR PIDGIN
	public static final int slangCR   	 = 101; // CREE
	public static final int slangCRH  	 = 102; // CRIMEAN TURKISH
	public static final int slangCRP  	 = 103; // CREOLE OR PIDGIN
	public static final int slangCS   	 = 104; // CZECH
	public static final int slangCSB  	 = 105; // KASHUBIAN
	public static final int slangCU   	 = 106; // CHURCH SLAVIC
	public static final int slangCUS  	 = 107; // CUSHITIC LANGUAGE
	public static final int slangCV   	 = 108; // CHUVASH
	public static final int slangCY   	 = 109; // WELSH
	public static final int slangDA   	 = 110; // DANISH
	public static final int slangDAK  	 = 111; // DAKOTA
	public static final int slangDAR  	 = 112; // DARGWA
	public static final int slangDAV  	 = 113; // TAITA
	public static final int slangDAY  	 = 114; // DAYAK
	public static final int slangDE_AT   = 115; // AUSTRIAN GERMAN
	public static final int slangDE_CH 	 = 116; // SWISS HIGH GERMAN
	public static final int slangDEL   	 = 117; // DELAWARE
	public static final int slangDEN   	 = 118; // SLAVE
	public static final int slangDGR   	 = 119; // DOGRIB
	public static final int slangDIN   	 = 120; // DINKA
	public static final int slangDJE   	 = 121; // ZARMA
	public static final int slangDOI   	 = 122; // DOGRI
	public static final int slangDRA   	 = 123; // DRAVIDIAN LANGUAGE
	public static final int slangDSB   	 = 124; // LOWER SORBIAN
	public static final int slangDUA   	 = 125; // DUALA
	public static final int slangDUM   	 = 126; // MIDDLE DUTCH
	public static final int slangDV    	 = 127; // DIVEHI
	public static final int slangDYO   	 = 128; // JOLA-FONYI
	public static final int slangDYU   	 = 129; // DYULA
	public static final int slangDZ    	 = 130; // DZONGKHA
	public static final int slangEBU   	 = 131; // EMBU
	public static final int slangEE    	 = 132; // EWE
	public static final int slangEFI   	 = 133; // EFIK
	public static final int slangEGY   	 = 134; // ANCIENT EGYPTIAN
	public static final int slangEKA   	 = 135; // EKAJUK
	public static final int slangEL    	 = 136; // GREEK
	public static final int slangELX   	 = 137; // ELAMITE
	public static final int slangEN_AU 	 = 138; // AUSTRALIAN ENGLISH
	public static final int slangEN_CA 	 = 139; // CANADIAN ENGLISH
	public static final int slangEN_GB 	 = 140; // BRITISH ENGLISH
	public static final int slangEN_US 	 = 141; // U.S. ENGLISH
	public static final int slangENM   	 = 142; // MIDDLE ENGLISH
	public static final int slangEO    	 = 143; // ESPERANTO
	public static final int slangES      = 144; // SPANISH
	public static final int slangES_419  = 145; // LATIN AMERICAN SPANISH
	public static final int slangES_ES   = 146; // IBERIAN SPANISH
	public static final int slangET      = 147; // ESTONIAN
	public static final int slangEU  	 = 148; // BASQUE
	public static final int slangEWO 	 = 149; // EWONDO
	public static final int slangFA  	 = 150; // PERSIAN
	public static final int slangFAN 	 = 151; // FANG
	public static final int slangFAT 	 = 152; // FANTI
	public static final int slangFF  	 = 153; // FULAH
	public static final int slangFI  	 = 154; // FINNISH
	public static final int slangFIL 	 = 155; // FILIPINO
	public static final int slangFIU 	 = 156; // FINNO-UGRIAN LANGUAGE
	public static final int slangFJ  	 = 157; // FIJIAN
	public static final int slangFO  	 = 158; // FAROESE
	public static final int slangFON 	 = 159; // FON
	public static final int slangFR      = 160; // FRENCH
	public static final int slangFR_CA 	 = 161; // CANADIAN FRENCH
	public static final int slangFR_CH 	 = 162; // SWISS FRENCH
	public static final int slangFRM   	 = 163; // MIDDLE FRENCH
	public static final int slangFRO   	 = 164; // OLD FRENCH
	public static final int slangFRR   	 = 165; // NORTHERN FRISIAN
	public static final int slangFRS   	 = 166; // EASTERN FRISIAN
	public static final int slangFUR   	 = 167; // FRIULIAN
	public static final int slangFY    	 = 168; // WESTERN FRISIAN
	public static final int slangGA    	 = 169; // IRISH
	public static final int slangGAA     = 170; // GA
	public static final int slangGAY 	 = 171; // GAYO
	public static final int slangGBA 	 = 172; // GBAYA
	public static final int slangGD  	 = 173; // SCOTTISH GAELIC
	public static final int slangGEM 	 = 174; // GERMANIC LANGUAGE
	public static final int slangGEZ 	 = 175; // GEEZ
	public static final int slangGIL 	 = 176; // GILBERTESE
	public static final int slangGL  	 = 177; // GALICIAN
	public static final int slangGMH 	 = 178; // MIDDLE HIGH GERMAN
	public static final int slangGN  	 = 179; // GUARANI
	public static final int slangGOH 	 = 180; // OLD HIGH GERMAN
	public static final int slangGON 	 = 181; // GONDI
	public static final int slangGOR 	 = 182; // GORONTALO
	public static final int slangGOT 	 = 183; // GOTHIC
	public static final int slangGRB 	 = 184; // GREBO
	public static final int slangGRC 	 = 185; // ANCIENT GREEK
	public static final int slangGSW 	 = 186; // SWISS GERMAN
	public static final int slangGU  	 = 187; // GUJARATI
	public static final int slangGUZ 	 = 188; // GUSII
	public static final int slangGV  	 = 189; // MANX
	public static final int slangGWI 	 = 190; // GWICH?IN
	public static final int slangHA  	 = 191; // HAUSA
	public static final int slangHAI 	 = 192; // HAIDA
	public static final int slangHAW 	 = 193; // HAWAIIAN
	public static final int slangHE  	 = 194; // HEBREW
	public static final int slangHI  	 = 195; // HINDI
	public static final int slangHIL 	 = 196; // HILIGAYNON
	public static final int slangHIM 	 = 197; // HIMACHALI
	public static final int slangHIT 	 = 198; // HITTITE
	public static final int slangHMN 	 = 199; // HMONG
	public static final int slangHO  	 = 200; // HIRI MOTU
	public static final int slangHR  	 = 201; // CROATIAN
	public static final int slangHSB 	 = 202; // UPPER SORBIAN
	public static final int slangHT  	 = 203; // HAITIAN
	public static final int slangHU  	 = 204; // HUNGARIAN
	public static final int slangHUP 	 = 205; // HUPA
	public static final int slangHY  	 = 206; // ARMENIAN
	public static final int slangHZ  	 = 207; // HERERO
	public static final int slangIA  	 = 208; // INTERLINGUA
	public static final int slangIBA 	 = 209; // IBAN
	public static final int slangID  	 = 210; // INDONESIAN
	public static final int slangIE  	 = 211; // INTERLINGUE
	public static final int slangIG  	 = 212; // IGBO
	public static final int slangII  	 = 213; // SICHUAN YI
	public static final int slangIJO 	 = 214; // IJO
	public static final int slangIK  	 = 215; // INUPIAQ
	public static final int slangILO 	 = 216; // ILOKO
	public static final int slangINC 	 = 217; // INDIC LANGUAGE
	public static final int slangINE 	 = 218; // INDO-EUROPEAN LANGUAGE
	public static final int slangINH 	 = 219; // INGUSH
	public static final int slangIO  	 = 220; // IDO
	public static final int slangIRA 	 = 221; // IRANIAN LANGUAGE
	public static final int slangIRO 	 = 222; // IROQUOIAN LANGUAGE
	public static final int slangIS  	 = 223; // ICELANDIC
	public static final int slangIT  	 = 224; // ITALIAN
	public static final int slangIU  	 = 225; // INUKTITUT
	public static final int slangJA  	 = 226; // JAPANESE
	public static final int slangJBO 	 = 227; // LOJBAN
	public static final int slangJMC 	 = 228; // MACHAME
	public static final int slangJPR 	 = 229; // JUDEO-PERSIAN
	public static final int slangJRB 	 = 230; // JUDEO-ARABIC
	public static final int slangJV  	 = 231; // JAVANESE
	public static final int slangKA  	 = 232; // GEORGIAN
	public static final int slangKAA 	 = 233; // KARA-KALPAK
	public static final int slangKAB 	 = 234; // KABYLE
	public static final int slangKAC 	 = 235; // KACHIN
	public static final int slangKAJ 	 = 236; // JJU
	public static final int slangKAM 	 = 237; // KAMBA
	public static final int slangKAR 	 = 238; // KAREN
	public static final int slangKAW 	 = 239; // KAWI
	public static final int slangKBD 	 = 240; // KABARDIAN
	public static final int slangKCG 	 = 241; // TYAP
	public static final int slangKDE 	 = 242; // MAKONDE
	public static final int slangKEA 	 = 243; // KABUVERDIANU
	public static final int slangKFO 	 = 244; // KORO
	public static final int slangKG  	 = 245; // KONGO
	public static final int slangKHA 	 = 246; // KHASI
	public static final int slangKHI 	 = 247; // KHOISAN LANGUAGE
	public static final int slangKHO 	 = 248; // KHOTANESE
	public static final int slangKHQ 	 = 249; // KOYRA CHIINI
	public static final int slangKI  	 = 250; // KIKUYU
	public static final int slangKJ  	 = 251; // KUANYAMA
	public static final int slangKK  	 = 252; // KAZAKH
	public static final int slangKL  	 = 253; // KALAALLISUT
	public static final int slangKLN 	 = 254; // KALENJIN
	public static final int slangKM  	 = 255; // KHMER
	public static final int slangKMB 	 = 256; // KIMBUNDU
	public static final int slangKN  	 = 257; // KANNADA
	public static final int slangKO  	 = 258; // KOREAN
	public static final int slangKOK 	 = 259; // KONKANI
	public static final int slangKOS 	 = 260; // KOSRAEAN
	public static final int slangKPE 	 = 261; // KPELLE
	public static final int slangKR  	 = 262; // KANURI
	public static final int slangKRC 	 = 263; // KARACHAY-BALKAR
	public static final int slangKRL 	 = 264; // KARELIAN
	public static final int slangKRO 	 = 265; // KRU
	public static final int slangKRU 	 = 266; // KURUKH
	public static final int slangKS  	 = 267; // KASHMIRI
	public static final int slangKSB 	 = 268; // SHAMBALA
	public static final int slangKSF 	 = 269; // BAFIA
	public static final int slangKSH 	 = 270; // COLOGNIAN
	public static final int slangKU  	 = 271; // KURDISH
	public static final int slangKUM 	 = 272; // KUMYK
	public static final int slangKUT 	 = 273; // KUTENAI
	public static final int slangKV  	 = 274; // KOMI
	public static final int slangKW  	 = 275; // CORNISH
	public static final int slangKY  	 = 276; // KIRGHIZ
	public static final int slangLAD 	 = 277; // LADINO
	public static final int slangLAG 	 = 278; // LANGI
	public static final int slangLAH 	 = 279; // LAHNDA
	public static final int slangLAM 	 = 280; // LAMBA
	public static final int slangLB  	 = 281; // LUXEMBOURGISH
	public static final int slangLEZ 	 = 282; // LEZGHIAN
	public static final int slangLG  	 = 283; // GANDA
	public static final int slangLI  	 = 284; // LIMBURGISH
	public static final int slangLN  	 = 285; // LINGALA
	public static final int slangLO  	 = 286; // LAO
	public static final int slangLOL 	 = 287; // MONGO
	public static final int slangLOZ 	 = 288; // LOZI
	public static final int slangLT  	 = 289; // LITHUANIAN
	public static final int slangLU  	 = 290; // LUBA-KATANGA
	public static final int slangLUA 	 = 291; // LUBA-LULUA
	public static final int slangLUI 	 = 292; // LUISENO
	public static final int slangLUN 	 = 293; // LUNDA
	public static final int slangLUO 	 = 294; // LUO
	public static final int slangLUS 	 = 295; // LUSHAI
	public static final int slangLUY 	 = 296; // LUYIA
	public static final int slangLV  	 = 297; // LATVIAN
	public static final int slangMAD 	 = 298; // MADURESE
	public static final int slangMAG 	 = 299; // MAGAHI
	public static final int slangMAI 	 = 300; // MAITHILI
	public static final int slangMAK 	 = 301; // MAKASAR
	public static final int slangMAN 	 = 302; // MANDINGO
	public static final int slangMAP 	 = 303; // AUSTRONESIAN LANGUAGE
	public static final int slangMAS 	 = 304; // MASAI
	public static final int slangMDF 	 = 305; // MOKSHA
	public static final int slangMDR 	 = 306; // MANDAR
	public static final int slangMEN 	 = 307; // MENDE
	public static final int slangMER 	 = 308; // MERU
	public static final int slangMFE 	 = 309; // MORISYEN
	public static final int slangMG  	 = 310; // MALAGASY
	public static final int slangMGA 	 = 311; // MIDDLE IRISH
	public static final int slangMGH 	 = 312; // MAKHUWA-MEETTO
	public static final int slangMH  	 = 313; // MARSHALLESE
	public static final int slangMI  	 = 314; // MAORI
	public static final int slangMIC 	 = 315; // MICMAC
	public static final int slangMIN 	 = 316; // MINANGKABAU
	public static final int slangMIS 	 = 317; // MISCELLANEOUS LANGUAGE
	public static final int slangMK  	 = 318; // MACEDONIAN
	public static final int slangMKH 	 = 319; // MON-KHMER LANGUAGE
	public static final int slangML  	 = 320; // MALAYALAM
	public static final int slangMN  	 = 321; // MONGOLIAN
	public static final int slangMNC 	 = 322; // MANCHU
	public static final int slangMNI 	 = 323; // MANIPURI
	public static final int slangMNO 	 = 324; // MANOBO LANGUAGE
	public static final int slangMO  	 = 325; // MOLDAVIAN
	public static final int slangMOH 	 = 326; // MOHAWK
	public static final int slangMOS 	 = 327; // MOSSI
	public static final int slangMR  	 = 328; // MARATHI
	public static final int slangMS  	 = 329; // MALAY
	public static final int slangMT  	 = 330; // MALTESE
	public static final int slangMUA 	 = 331; // MUNDANG
	public static final int slangMUL 	 = 332; // MULTIPLE LANGUAGES
	public static final int slangMUN 	 = 333; // MUNDA LANGUAGE
	public static final int slangMUS 	 = 334; // CREEK
	public static final int slangMWL 	 = 335; // MIRANDESE
	public static final int slangMWR 	 = 336; // MARWARI
	public static final int slangMY  	 = 337; // BURMESE
	public static final int slangMYN 	 = 338; // MAYAN LANGUAGE
	public static final int slangMYV 	 = 339; // ERZYA
	public static final int slangNA  	 = 340; // NAURU
	public static final int slangNAH 	 = 341; // NAHUATL
	public static final int slangNAI 	 = 342; // NORTH AMERICAN INDIAN LANGUAGE
	public static final int slangNAP 	 = 343; // NEAPOLITAN
	public static final int slangNAQ 	 = 344; // NAMA
	public static final int slangNB  	 = 345; // NORWEGIAN BOKMAL
	public static final int slangND  	 = 346; // NORTH NDEBELE
	public static final int slangNDS 	 = 347; // LOW GERMAN
	public static final int slangNE  	 = 348; // NEPALI
	public static final int slangNEW 	 = 349; // NEWARI
	public static final int slangNG  	 = 350; // NDONGA
	public static final int slangNIA 	 = 351; // NIAS
	public static final int slangNIC 	 = 352; // NIGER-KORDOFANIAN LANGUAGE
	public static final int slangNIU 	 = 353; // NIUEAN
	public static final int slangNL  	 = 354; // DUTCH
	public static final int slangNL_BE   = 355; // FLEMISH
	public static final int slangNMG   	 = 356; // KWASIO
	public static final int slangNN    	 = 357; // NORWEGIAN NYNORSK
	public static final int slangNO    	 = 358; // NORWEGIAN
	public static final int slangNOG   	 = 359; // NOGAI
	public static final int slangNON   	 = 360; // OLD NORSE
	public static final int slangNQO   	 = 361; // N’KO
	public static final int slangNR    	 = 362; // SOUTH NDEBELE
	public static final int slangNSO   	 = 363; // NORTHERN SOTHO
	public static final int slangNUB   	 = 364; // NUBIAN LANGUAGE
	public static final int slangNUS   	 = 365; // NUER
	public static final int slangNV    	 = 366; // NAVAJO
	public static final int slangNWC   	 = 367; // CLASSICAL NEWARI
	public static final int slangNY    	 = 368; // NYANJA
	public static final int slangNYM   	 = 369; // NYAMWEZI
	public static final int slangNYN   	 = 370; // NYANKOLE
	public static final int slangNYO   	 = 371; // NYORO
	public static final int slangNZI   	 = 372; // NZIMA
	public static final int slangOC    	 = 373; // OCCITAN
	public static final int slangOJ    	 = 374; // OJIBWA
	public static final int slangOM    	 = 375; // OROMO
	public static final int slangOR    	 = 376; // ORIYA
	public static final int slangOS    	 = 377; // OSSETIC
	public static final int slangOSA   	 = 378; // OSAGE
	public static final int slangOTA   	 = 379; // OTTOMAN TURKISH
	public static final int slangOTO   	 = 380; // OTOMIAN LANGUAGE
	public static final int slangPA    	 = 381; // PUNJABI
	public static final int slangPAA   	 = 382; // PAPUAN LANGUAGE
	public static final int slangPAG   	 = 383; // PANGASINAN
	public static final int slangPAL   	 = 384; // PAHLAVI
	public static final int slangPAM   	 = 385; // PAMPANGA
	public static final int slangPAP   	 = 386; // PAPIAMENTO
	public static final int slangPAU   	 = 387; // PALAUAN
	public static final int slangPEO   	 = 388; // OLD PERSIAN
	public static final int slangPHI   	 = 389; // PHILIPPINE LANGUAGE
	public static final int slangPHN   	 = 390; // PHOENICIAN
	public static final int slangPI    	 = 391; // PALI
	public static final int slangPL    	 = 392; // POLISH
	public static final int slangPON   	 = 393; // POHNPEIAN
	public static final int slangPRA   	 = 394; // PRAKRIT LANGUAGE
	public static final int slangPRO   	 = 395; // OLD PROVENCAL
	public static final int slangPS    	 = 396; // PASHTO
	public static final int slangPT    	 = 397; // PORTUGUESE
	public static final int slangPT_BR 	 = 398; // BRAZILIAN PORTUGUESE
	public static final int slangPT_PT 	 = 399; // IBERIAN PORTUGUESE
	public static final int slangQU    	 = 400; // QUECHUA
	public static final int slangRAJ   	 = 401; // RAJASTHANI
	public static final int slangRAP   	 = 402; // RAPANUI
	public static final int slangRAR   	 = 403; // RAROTONGAN
	public static final int slangRM    	 = 404; // ROMANSH
	public static final int slangRN    	 = 405; // RUNDI
	public static final int slangRO    	 = 406; // ROMANIAN
	public static final int slangROA   	 = 407; // ROMANCE LANGUAGE
	public static final int slangROF   	 = 408; // ROMBO
	public static final int slangROM   	 = 409; // ROMANY
	public static final int slangROOT  	 = 410; // ROOT
	public static final int slangRUP   	 = 411; // AROMANIAN
	public static final int slangRW    	 = 412; // KINYARWANDA
	public static final int slangRWK   	 = 413; // RWA
	public static final int slangSA    	 = 414; // SANSKRIT
	public static final int slangSAD   	 = 415; // SANDAWE
	public static final int slangSAH   	 = 416; // SAKHA
	public static final int slangSAI   	 = 417; // SOUTH AMERICAN INDIAN LANGUAGE
	public static final int slangSAL   	 = 418; // SALISHAN LANGUAGE
	public static final int slangSAM   	 = 419; // SAMARITAN ARAMAIC
	public static final int slangSAQ   	 = 420; // SAMBURU
	public static final int slangSAS   	 = 421; // SASAK
	public static final int slangSAT   	 = 422; // SANTALI
	public static final int slangSBP   	 = 423; // SANGU
	public static final int slangSC    	 = 424; // SARDINIAN
	public static final int slangSCN   	 = 425; // SICILIAN
	public static final int slangSCO   	 = 426; // SCOTS
	public static final int slangSD    	 = 427; // SINDHI
	public static final int slangSE    	 = 428; // NORTHERN SAMI
	public static final int slangSEE   	 = 429; // SENECA
	public static final int slangSEH   	 = 430; // SENA
	public static final int slangSEL   	 = 431; // SELKUP
	public static final int slangSEM   	 = 432; // SEMITIC LANGUAGE
	public static final int slangSES   	 = 433; // KOYRABORO SENNI
	public static final int slangSG    	 = 434; // SANGO
	public static final int slangSGA   	 = 435; // OLD IRISH
	public static final int slangSGN   	 = 436; // SIGN LANGUAGE
	public static final int slangSH    	 = 437; // SERBO-CROATIAN
	public static final int slangSHI   	 = 438; // TACHELHIT
	public static final int slangSHN   	 = 439; // SHAN
	public static final int slangSI    	 = 440; // SINHALA
	public static final int slangSID   	 = 441; // SIDAMO
	public static final int slangSIO   	 = 442; // SIOUAN LANGUAGE
	public static final int slangSIT   	 = 443; // SINO-TIBETAN LANGUAGE
	public static final int slangSK    	 = 444; // SLOVAK
	public static final int slangSL    	 = 445; // SLOVENIAN
	public static final int slangSLA   	 = 446; // SLAVIC LANGUAGE
	public static final int slangSM    	 = 447; // SAMOAN
	public static final int slangSMA   	 = 448; // SOUTHERN SAMI
	public static final int slangSMI   	 = 449; // SAMI LANGUAGE
	public static final int slangSMJ   	 = 450; // LULE SAMI
	public static final int slangSMN   	 = 451; // INARI SAMI
	public static final int slangSMS   	 = 452; // SKOLT SAMI
	public static final int slangSN    	 = 453; // SHONA
	public static final int slangSNK   	 = 454; // SONINKE
	public static final int slangSO    	 = 455; // SOMALI
	public static final int slangSOG   	 = 456; // SOGDIEN
	public static final int slangSON   	 = 457; // SONGHAI
	public static final int slangSQ    	 = 458; // ALBANIAN
	public static final int slangSR    	 = 459; // SERBIAN
	public static final int slangSRN   	 = 460; // SRANAN TONGO
	public static final int slangSRR   	 = 461; // SERER
	public static final int slangSS    	 = 462; // SWATI
	public static final int slangSSA   	 = 463; // NILO-SAHARAN LANGUAGE
	public static final int slangSSY   	 = 464; // SAHO
	public static final int slangST    	 = 465; // SOUTHERN SOTHO
	public static final int slangSU    	 = 466; // SUNDANESE
	public static final int slangSUK   	 = 467; // SUKUMA
	public static final int slangSUS   	 = 468; // SUSU
	public static final int slangSUX   	 = 469; // SUMERIAN
	public static final int slangSV    	 = 470; // SWEDISH
	public static final int slangSW    	 = 471; // SWAHILI
	public static final int slangSWB   	 = 472; // COMORIAN
	public static final int slangSWC   	 = 473; // CONGO SWAHILI
	public static final int slangSYC   	 = 474; // CLASSICAL SYRIAC
	public static final int slangSYR   	 = 475; // SYRIAC
	public static final int slangTA    	 = 476; // TAMIL
	public static final int slangTAI   	 = 477; // TAI LANGUAGE
	public static final int slangTE    	 = 478; // TELUGU
	public static final int slangTEM   	 = 479; // TIMNE
	public static final int slangTEO   	 = 480; // TESO
	public static final int slangTER   	 = 481; // TERENO
	public static final int slangTET   	 = 482; // TETUM
	public static final int slangTG    	 = 483; // TAJIK
	public static final int slangTH    	 = 484; // THAI
	public static final int slangTI    	 = 485; // TIGRINYA
	public static final int slangTIG   	 = 486; // TIGRE
	public static final int slangTIV   	 = 487; // TIV
	public static final int slangTK    	 = 488; // TURKMEN
	public static final int slangTKL   	 = 489; // TOKELAU
	public static final int slangTL    	 = 490; // TAGALOG
	public static final int slangTLH   	 = 491; // KLINGON
	public static final int slangTLI   	 = 492; // TLINGIT
	public static final int slangTMH   	 = 493; // TAMASHEK
	public static final int slangTN    	 = 494; // TSWANA
	public static final int slangTO    	 = 495; // TONGAN
	public static final int slangTOG   	 = 496; // NYASA TONGA
	public static final int slangTPI   	 = 497; // TOK PISIN
	public static final int slangTR    	 = 498; // TURKISH
	public static final int slangTRV   	 = 499; // TAROKO
	public static final int slangTS    	 = 500; // TSONGA
	public static final int slangTSI   	 = 501; // TSIMSHIAN
	public static final int slangTT    	 = 502; // TATAR
	public static final int slangTUM   	 = 503; // TUMBUKA
	public static final int slangTUP   	 = 504; // TUPI LANGUAGE
	public static final int slangTUT   	 = 505; // ALTAIC LANGUAGE
	public static final int slangTVL   	 = 506; // TUVALU
	public static final int slangTW    	 = 507; // TWI
	public static final int slangTWQ   	 = 508; // TASAWAQ
	public static final int slangTY    	 = 509; // TAHITIAN
	public static final int slangTYV   	 = 510; // TUVINIAN
	public static final int slangTZM   	 = 511; // CENTRAL MOROCCO TAMAZIGHT
	public static final int slangUDM   	 = 512; // UDMURT
	public static final int slangUG    	 = 513; // UIGHUR
	public static final int slangUGA   	 = 514; // UGARITIC
	public static final int slangUK    	 = 515; // UKRAINIAN
	public static final int slangUMB   	 = 516; // UMBUNDU
	public static final int slangUND   	 = 517; // UNKNOWN LANGUAGE
	public static final int slangUR    	 = 518; // URDU
	public static final int slangUZ    	 = 519; // UZBEK
	public static final int slangVAI   	 = 520; // VAI
	public static final int slangVE    	 = 521; // VENDA
	public static final int slangVI    	 = 522; // VIETNAMESE
	public static final int slangVO    	 = 523; // VOLAPUK
	public static final int slangVOT   	 = 524; // VOTIC
	public static final int slangVUN   	 = 525; // VUNJO
	public static final int slangWA    	 = 526; // WALLOON
	public static final int slangWAE   	 = 527; // WALSER
	public static final int slangWAK   	 = 528; // WAKASHAN LANGUAGE
	public static final int slangWAL   	 = 529; // WALAMO
	public static final int slangWAR   	 = 530; // WARAY
	public static final int slangWAS   	 = 531; // WASHO
	public static final int slangWEN   	 = 532; // SORBIAN LANGUAGE
	public static final int slangWO    	 = 533; // WOLOF
	public static final int slangXAL   	 = 534; // KALMYK
	public static final int slangXH    	 = 535; // XHOSA
	public static final int slangXOG   	 = 536; // SOGA
	public static final int slangYAO   	 = 537; // YAO
	public static final int slangYAP   	 = 538; // YAPESE
	public static final int slangYAV   	 = 539; // YANGBEN
	public static final int slangYI    	 = 540; // YIDDISH
	public static final int slangYO    	 = 541; // YORUBA
	public static final int slangYPK   	 = 542; // YUPIK LANGUAGE
	public static final int slangYUE   	 = 543; // CANTONESE
	public static final int slangZA    	 = 544; // ZHUANG
	public static final int slangZAP   	 = 545; // ZAPOTEC
	public static final int slangZBL   	 = 546; // BLISSYMBOLS
	public static final int slangZEN   	 = 547; // ZENAGA
	public static final int slangZH    	 = 548; // CHINESE
	public static final int slangZH_HANS = 549; // SIMPLIFIED CHINESE
	public static final int slangZH_HANT = 550; // TRADITIONAL CHINESE
	public static final int slangZND     = 551; // ZANDE
	public static final int slangZU      = 552; // ZULU
	public static final int slangZUN     = 553; // ZUNI
	public static final int slangZXX     = 554; // NO LINGUISTIC CONTENT
	public static final int slangZZA     = 555; // ZAZA
	//
	// @persistent
	// Протоколы обмена данными
	// Аналог SLIB InetUrl::protXXX
	//
	public static final int uripprotUnkn      =  0;
	public static final int uripprotHttp      =  1;  // http
	public static final int uripprotHttps     =  2;  // https
	public static final int uripprotFtp       =  3;  // ftp
	public static final int uripprotGopher    =  4;  // gopher
	public static final int uripprotMailto    =  5;  // mailto
	public static final int uripprotNews      =  6;
	public static final int uripprotNntp      =  7;
	public static final int uripprotIrc       =  8;
	public static final int uripprotProspero  =  9;
	public static final int uripprotTelnet    = 10;
	public static final int uripprotWais      = 11;
	public static final int uripprotXmpp      = 12;
	public static final int uripprotFile      = 13;
	public static final int uripprotData      = 14;
	public static final int uripprotSvn       = 15;
	public static final int uripprotSocks4    = 16;
	public static final int uripprotSocks5    = 17;
	public static final int uripprotSMTP      = 18; // Протокол отправки почты
	public static final int uripprotSMTPS     = 19; // Протокол отправки почты (SSL)
	public static final int uripprotPOP3      = 20; // Протокол получения почты
	public static final int uripprotPOP3S     = 21; // Протокол получения почты (SSL)
	public static final int uripprotIMAP      = 22; // Internet Message Access Protocol
	public static final int uripprotIMAPS     = 23; // Internet Message Access Protocol (SSL)
	public static final int uripprotFtps      = 24; // ftps
	public static final int uripprotTFtp      = 25;
	public static final int uripprotDict      = 26;
	public static final int uripprotSSH       = 27;
	public static final int uripprotSMB       = 28;
	public static final int uripprotSMBS      = 29;
	public static final int uripprotRTSP      = 30;
	public static final int uripprotRTMP      = 31;
	public static final int uripprotRTMPT     = 32;
	public static final int uripprotRTMPS     = 33;
	public static final int uripprotLDAP      = 34;
	public static final int uripprotLDAPS     = 35;
	public static final int uripprotMailFrom  = 36; // Фиктивный протокол (не имеющий соответствия в стандартах). Применяется
	// для внутреннего представления описания параметров приема данных из почтовых сообщений.
	public static final int uripprot_p_PapyrusServer = 37; // private-протокол системы Papyrus
	public static final int uripprotAMQP      = 38; //
	public static final int uripprotAMQPS     = 39; //
	public static final int uripprot_p_MYSQL  = 40; // private: идентифицирует url доступа к серверу MySQL
	public static final int uripprot_p_SQLITE = 41; // private: идентифицирует url доступа к базе данных SQLite
	public static final int uripprot_p_ORACLE = 42; // private: идентифицирует url доступа к серверу базы данных ORACLE
	public static final int uripprotGit       = 43; // @v11.1.11
	//
	//
	//
	public static final int GENDER_UNDEF       = 0; // Не определена (в принципе не известно из-за того, что не было попыток идентифицировать)
	public static final int GENDER_MALE        = 1; // Мужской
	public static final int GENDER_FEMALE      = 2; // Женский
	public static final int GENDER_QUESTIONING = 3; // Под вопросом (попытка идентификации пола была, но окончилась неудачей)
	//
	//
	//
	public static final String dow_en_sh[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
	public static final String mon_en_sh[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	public static final String dow_en[] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };
	//
	// SLIB Error Codes
	//
	public static final int SLERR_SUCCESS                   = 0;
	public static final int SLERR_NOMEM                     = 1;
	public static final int SLERR_INVRANGE                  = 2;
	public static final int SLERR_BOUNDS                    = 3;
	public static final int SLERR_BUFTOOSMALL               = 4;
	public static final int SLERR_NOFOUND                   = 5;
	public static final int SLERR_INVSYMBOL                 = 6;
	public static final int SLERR_INVDATA                   = 7;
	public static final int SLERR_EOLEXPECTED               = 8;
	public static final int SLERR_DATAEXPECTED              = 9;
	public static final int SLERR_TERMEXPECTED              = 10;
	public static final int SLERR_INVDAY                    = 11;
	public static final int SLERR_INVMONTH                  = 12;
	public static final int SLERR_INVYEAR                   = 13;
	public static final int SLERR_ARYDELTA                  = 15;
	public static final int SLERR_ARYZERODELTA              = 16;
	public static final int SLERR_ARYNOTORD                 = 17;
	public static final int SLERR_ARYITEMNFOUND             = SLERR_NOFOUND;
	public static final int SLERR_ARYDUPITEM                = 18;
	public static final int SLERR_ARYZEROCOUNT              = 19;
	public static final int SLERR_FILENOTFOUND              = 20;
	public static final int SLERR_READFAULT                 = 21;
	public static final int SLERR_WRITEFAULT                = 22;
	public static final int SLERR_INVFORMAT                 = 23;
	//
	// Printer errors
	//
	public static final int SLERR_PRTNOTREADY               = 24;
	public static final int SLERR_PRTBUSY                   = 25;
	public static final int SLERR_PRTOUTOFPAPER             = 26;
	public static final int SLERR_REZNFOUND                 = 27;
	public static final int SLERR_PGLENTOOSMALL             = 28; // Page lenght too small for report
	public static final int SLERR_NODEFPRINTER              = 29; // Не удалось идентифицировать принтер по умолчанию

	public static final int SLERR_OPENFAULT                 = 30; // Error opening file
	public static final int SLERR_MKDIRFAULT                = 31; // Error creating directory
	public static final int SLERR_INVALIDCRC                = 32; // Error recovering file (invalid CRC)
	public static final int SLERR_CANTCOMPRESS              = 33; // Ошибка сжатия //
	public static final int SLERR_CANTDECOMPRESS            = 34; // Ошибка распаковки
	public static final int SLERR_DISKFULL                  = 35; // Диск переполнен
	public static final int SLERR_WINDOWS                   = 36; // Ошибка Win32
	public static final int SLERR_INVGUIDSTR                = 37; // Недопустимая строка GUID (%s)
	public static final int SLERR_INVSERIALIZEVER           = 38; // Считан недопустимый номер версии сериализации (%s)
	public static final int SLERR_DLLLOADFAULT              = 39; // Ошибка загрузки DLL %s
	//
	// Communication errors
	//
	public static final int SLERR_COMMINIT                  = 40; // Error initializing comm port %s
	public static final int SLERR_COMMSEND                  = 41; // Error sending data to comm port
	public static final int SLERR_COMMRCV                   = 42; // Error receiving data from comm port
	public static final int SLERR_HTTPCODE                  = 43; // HTTP Error code=%s
	public static final int	SLERR_USB                       = 44; // Ошибка usb: %s
	//
	// Socket errors
	//
	public static final int SLERR_SOCK_UNABLEOPEN           = 50; // Ошибка открытия сокета
	public static final int SLERR_SOCK_CONNECT              = 51; // Ошибка установки соединения с хостом '%s'
	public static final int SLERR_SOCK_HOSTRESLVFAULT       = 52; // Ошибка разрешения имени хоста
	public static final int SLERR_SOCK_TIMEOUT              = 53; // Превышен таймаут ожидания сокета
	public static final int SLERR_SOCK_OPTERR               = 54; // Ошибка состояния сокета
	public static final int SLERR_SOCK_NONBLOCKINGCONN      = 55; // Ошибка неблокового соединени с хостом
	public static final int SLERR_SOCK_NONBLOCKINGWR        = 56; // Ошибка неблоковой записи в сокет
	public static final int SLERR_SOCK_NONBLOCKINGRD        = 57; // Ошибка неблокового чтения из сокета
	public static final int SLERR_SOCK_SEND                 = 58; // Ошибка записи в сокет
	public static final int SLERR_SOCK_RECV                 = 59; // Ошибка чтения из сокета
	public static final int SLERR_SOCK_CONNCLOSEDBYHOST     = 60; // Соединение закрыто хостом
	public static final int SLERR_SOCK_LINETOOLONG          = 61; // Считанная строка слишком велика
	public static final int SLERR_SOCK_FILETRANSMFAULT      = 62; // Ошибка передачи файла
	public static final int SLERR_INVPATH                   = 63; // Неправильный формат пути
	public static final int SLERR_SOCK_ACCEPT               = 64; // Ошибка при создании соединения //
	public static final int SLERR_SOCK_LISTEN               = 65; // Ошибка при создании соединения //
	public static final int SLERR_SOCK_WINSOCK              = 66; // Ошибка WinSock (call WSAGetLastError)
	//
	// Matrix error
	//
	public static final int SLERR_MTX_INCOMPATDIM_VADD     =  70; // Incompatible dim in vect+vect op
	public static final int SLERR_MTX_INCOMPATDIM_VIMX     =  71; // Incompatible dim in vect to matrix op
	public static final int SLERR_MTX_INCOMPATDIM_MVMUL    =  72; // Incompatible dim in matrix*vector op
	public static final int SLERR_MTX_INCOMPATDIM_MMMUL    =  73; // Incompatible dim in matrix*matrix op
	public static final int SLERR_MTX_INCOMPATDIM_MMADD    =  74; // Incompatible dim in matrix+matrix op
	public static final int SLERR_MTX_INVERSE              =  75; // Ошибка обращения матрицы.

	public static final int SLERR_INVPARAM                 =  96; // Недопустимый параметр функции (%s)
	public static final int SLERR_FILESHARINGVIOLATION     =  97; // Ошибка совместного доступа к файлу
	public static final int SLERR_FLOCKFAULT               =  98; // Ошибка блокировки файла //
	public static final int SLERR_USERBREAK                =  99; // Операция прервана пользователем
	public static final int SLERR_FILENOTOPENED            = 100; // Файл не открыт

	public static final int SLERR_TXTDB_ZEROLENFIXEDFLD    = 101; // Для текстового файла с фиксированными полями должны быть определены длины всех полей
	public static final int SLERR_TXTDB_EMPTYFLDDIV        = 102; // Для текстового файла не определен разделитель полей
	public static final int SLERR_TXTDB_EMPTYVERTTERM      = 103; // Для текстового файла с вертикальной раскладкой не определ терминатор записей
	public static final int SLERR_TXTDB_MISSPLHEADER       = 104; // Повторная попытка добавить заголовок к файлу, либо уже добавлены записи (%s)
	public static final int SLERR_XMLDB_INVRECORROOTTAG    = 110; // Для xml файла не определен корневой тег или тег записи
	public static final int SLERR_XMLDB_ROOTTAGEMPTY       = 111; //
	public static final int SLERR_XMLDB_RECTAGEMPTY        = 112; //
	public static final int SLERR_XMLDB_ROOTTAGINVCHR      = 113; //
	public static final int SLERR_XMLDB_RECTAGINVCHR       = 114; //
	public static final int SLERR_EXCL_SAVEFAULT           = 115; // @v10.9.6 Ошибка сохранения файла Excel '%s'

	public static final int SLERR_SDREC_DUPFLDID           = 121; // Поле с заданным идентификатором в записи уже присутствует ('%s')
	public static final int SLERR_SDREC_DUPFLDNAME         = 122; // Поле с заданным именем '%s' в записи уже присутствует
	public static final int SLERR_SDREC_SYNTAX             = 123; // Ошибка синтаксиса при разборе определения типа поля (%s) //
	public static final int SLERR_SDREC_FLDIDNFOUND        = 124; // Поле с ид=%s в записи отсутствует
	public static final int SLERR_DUPIDXSEG                = 125; // Дублирование поля в индексе
	public static final int SLERR_DBF_INVHEADER            = 126; // Нарушена целостность заголовка DBF-файла '%s'
	public static final int SLERR_DBF_NOTOPENED            = 127; // Таблица DBF '%s' не открыта
	public static final int SLERR_DBF_INVRECNO             = 128; // Недопустимый номер записи при доступе к DBF-таблице '%s'
	public static final int SLERR_SPII_CMDHDRREADFAULT     = 129; // Ошибка чтения заголовка команды StyloPalm: %s
	public static final int SLERR_SPII_CMDREADFAULT        = 130; // Ошибка чтения тела команды StyloPalm: %s
	//
	// CRegExp error
	//
	public static final int SLERR_RE_UNINIT                = 150; // Регулярное выражение не инициализировано
	public static final int SLERR_RE_NOEXPR                = 151; // CoolRegexp::compile(): No expression supplied.\n");
	public static final int SLERR_RE_EXPRTOOBIG            = 152; // CoolRegexp::compile(): Expression too big.\n");
	public static final int SLERR_RE_NOMEM                 = 153; // CoolRegexp::compile(): Out of memory.\n");
	public static final int SLERR_RE_INVBRANGE             = 154; // CoolRegexp::compile(): Invalid range in [].\n");
	public static final int SLERR_RE_UNMATCHEDBR           = 155; // CoolRegexp::compile(): Unmatched [].\n");
	public static final int SLERR_RE_INTERNAL              = 156; // CoolRegexp::compile(): Internal error.\n"); /* Never here */
	public static final int SLERR_RE_WCNOTHFOLLOWS         = 157; // CoolRegexp::compile(): ?+* follows nothing.\n");
	public static final int SLERR_RE_TRAILINGBSL           = 158; // CoolRegexp::compile(): Trailing backslash.\n");
	public static final int SLERR_RE_WCCOULDBEEMPT         = 159; // CoolRegexp::compile() : *+ operand could be empty.\n");
	public static final int SLERR_RE_NESTEDWC              = 160; // CoolRegexp::compile(): Nested *?+.\n");
	public static final int SLERR_RE_TOOMANYPAR            = 161; // CoolRegexp::compile(): Too many parentheses.\n");
	public static final int SLERR_RE_UNMATCHPAR            = 162; // CoolRegexp::compile(): Unmatched parentheses.\n");
	public static final int SLERR_RE_BUFCORRUPT            = 163; // CoolRegexp::find(): Compiled regular expression corrupted.\n");
	public static final int SLERR_RE_MEMCORRUPT            = 164; // CoolRegexp::find(): Internal error -- memory corrupted.\n");
	public static final int SLERR_RE_PTRCORRUPT            = 165; // CoolRegexp::find(): Internal error -- corrupted pointers.\n");

	public static final int SLERR_HT_NOASSOC               = 180; // Экземпляр хэш-таблицы не поддерживает ассоциации
	public static final int SLERR_FILE_DELETE              = 181; // Ошибка удаления файла '%s'
	public static final int SLERR_FILE_RENAME              = 182; // Ошибка переименования файла '%s'
	public static final int SLERR_INIOPENFAULT             = 183; // Ошибка открытия INI-файла '%s'
	public static final int SLERR_PAINTTOOLIDBUSY          = 184; // Идентификатор PaintToolBox занят
	public static final int SLERR_INVHOUR                  = 185; // Недопустимый час во времени
	public static final int SLERR_INVMIN                   = 186; // Недопустимые минуты во времени
	public static final int SLERR_INVSEC                   = 187; // Недопустимые секунды во времени
	public static final int SLERR_INVTSEC                  = 188; // Недопустимые сотые секунд во времени
	public static final int SLERR_UNMATCHSTREAMARRAY       = 189; // Несоответствующий размер записи считываемого массива данных (%s)
	public static final int SLERR_INVGEOLATITUDE           = 190; // Недопустимое значение географической широты
	public static final int SLERR_INVGEOLONGITUDE          = 191; // Недопустимое значение географической долготы
	public static final int SLERR_SRLZ_UNDEFSUPPDATE       = 192; // Для восстановления даты из потока нет необходимой опорной даты
	public static final int SLERR_SRLZ_INVDATAIND          = 193; // Недопустимое значение индикатора данных при восстановлении из потока
	public static final int SLERR_SRLZ_UNEQFLDLIST         = 194; // Ассоциированная с данными структура не равна заданной в контексте
	public static final int SLERR_SRLZ_COMMRDFAULT         = 195; // Общий сбой при чтении данных (данные не соответствуют ожидаемым значениям)
	public static final int SLERR_SBUFRDSIZE               = 196; // Ошибка чтения из буфера: размер считанных данных не равен запрошенному (%s)
	public static final int SLERR_BINSET_SRCIDNFOUND       = 197; // Ошибка SBinarySet: в исходном пуле отсутствует отрезок с идентификатором %s
	public static final int SLERR_BINSET_UNSRLZ_SIGNATURE  = 198; // Ошибка чтения SBinarySet: неверный заголовок или сигнатура
	public static final int SLERR_BINSET_UNSRLZ_ITEMHDR    = 199; // Ошибка чтения SBinarySet: инвалидный заголовок элемента

	public static final int SLERR_WINSVC_COMMON            = 200; // Ошибка Windows Service
	public static final int SLERR_WINSVC_SVCNEXISTS        = 201; // Windows Service '%s' не существует
	public static final int SLERR_WINSEC_ACQCREDHDL        = 202; // "AcquireCredentialsHandle failed with %s"
	public static final int SLERR_WINSEC_INITSECCTX        = 203; // "InitializeSecurityContext failed with %s"
	public static final int SLERR_WINSEC_COMPLAUTHTOK      = 204; // "CompleteAuthToken failed with %s"
	public static final int SLERR_WINSEC_COMPLAUTHTOKNSUPP = 205; // "CompleteAuthToken not supported"
	public static final int SLERR_WINSEC_ACCPTSECCTX       = 206; // "AcceptSecurityContext failed with %s"
	public static final int SLERR_WMI_CREATELOCATOR        = 210; // Ошибка создания WMI-локатора
	public static final int SLERR_WMI_CONNECTSRV           = 211; // Ошибка соединения с WMI-сервером '%s'
	public static final int SLERR_WMI_SETPROXIBLANKET      = 212; // Ошибка вызова метода CoSetProxyBlanket
	public static final int SLERR_WMI_GETOBJECT            = 213; // Ошибка вызова метода WMI GetObject (%s)
	public static final int SLERR_WMI_GETMETHOD            = 214; // Ошибка вызова метода WMI GetMethod (%s)
	public static final int SLERR_WMI_SPAWNINSTANCE        = 215; // Ошибка вызова метода WMI SpawnInstance
	public static final int SLERR_WMI_EXECMETHOD           = 216; // Ошибка вызова метода WMI ExecMethod '%s'
	public static final int SLERR_WMI_EXECMETHODRETVAL     = 217; // Код возврата созданного процесса '%s' не равен 0
	public static final int SLERR_GLOBOBJIDXNFOUNT         = 218; // Не найден глобальный объект SLib по индексу %s
	public static final int SLERR_LANGSYMBNFOUND           = 219; // Не найден символ языка '%s'
	public static final int SLERR_DISPIFCCLI               = 220; // Ошибка вызова dispatch-интерфейса %s
	//
	//
	//
	public static final int SLERR_MATH_DOMAIN              = 300; // Ошибка области определения функции (sqrt(-1))
	public static final int SLERR_MATH_ROUND               = 301; // Ошибка округления //
	public static final int SLERR_MATH_OVERFLOW            = 302; // Ошибка переполнения ieee-числа
	public static final int SLERR_MATH_UNDERFLOW           = 303; // Ошибка переполнения нижней границы точности ieee-числа
	public static final int SLERR_MATH_MAXITER             = 304; // Ошибка превышения максимального числа итераций
	public static final int SLERR_MATH_TSVECBYSYMBNFOUND   = 305; // Вектор временной серии с символом '%s' не найден
	public static final int SLERR_BASE64DECODEFAULT        = 306; // Ошибка декодирования base64
	//
	//
	//
	public static final int SLERR_INVDBSYMB                = 400; // Неверный символ базы данных '%s'
	public static final int SLERR_UTFCVT_SRCEXHAUSTED      = 401; // Не полный входной буфер UTF8
	public static final int SLERR_UTFCVT_ILLUTF8           = 402; // Недопустимый входной символ UTF8
	public static final int SLERR_UTFCVT_ILLUTF16          = 403; // Недопустимый входной символ UTF16
	public static final int SLERR_TAB_NFOUND               = 404; // Таблица STab '%s' не найдена
	public static final int SLERR_INVIMAGEPIXFORMAT        = 405; // Недопустимый формат пикселя.
	public static final int SLERR_IMAGEFILENOTJPEG         = 406; // Файл '%s' не является JPEG-файлом.
	public static final int SLERR_JPEGLOADFAULT            = 407; // Ошибка загрузки JPEG-Файла '%s'
	public static final int SLERR_IMAGEFILENOTPNG          = 408; // Файл '%s' не является PNG-файлом.
	public static final int SLERR_PNGLOADFAULT             = 409; // Ошибка загрузки PNG-изображения: '%s'
	public static final int SLERR_UNSUPPIMGFILEFORMAT      = 410; // Не поддерживаемый или недопустимый формат файла изображения: '%s'
	public static final int SLERR_SFILRDSIZE               = 411; // Ошибка чтения из файла: размер считанных данных не равен запрошенному (%s)
	public static final int SLERR_IMAGEFILENOTICO          = 412; // Файл '%s' не является ICO-файлом.
	public static final int SLERR_PNGSTOREFAULT            = 413; // Ошибка сохранения PNG-изображения: '%s'
	public static final int SLERR_INVSDRAWFIGKIND          = 414; // Internal: Недопустимый вид графической фигуры (%s)
	public static final int SLERR_DUPDRAWGROUPSYMB         = 415; // Дублирование символа '%s' элемента, добавляемого в SDrawGroup
	public static final int SLERR_WTMTA_UNDEFFIG           = 416; // Для элемента TWhatmanToolArray '%s' не заданы ни фигура, ни иконка.
	public static final int SLERR_WTMTA_BADCRC             = 417; // Нарушена целостность WTA-файла '%s'
	public static final int SLERR_IMAGEFILENOTBMP          = 418; // Файл '%s' не является BMP-файлом.
	public static final int SLERR_INVBMPHEADER             = 419; // Недопустимый BMP-заголок (%s)
	public static final int SLERR_BMPCOMPRNSUPPORTED       = 420; // Сжатые BMP-изображения не поддерживаются.
	public static final int SLERR_SOAPR_UNDEFREF           = 421; // Неопределенная ссылка '%s' в SOAP-результате
	public static final int SLERR_SOAPR_UNDEFTYPE          = 422; // Неопределенный тип данных '%s' в SOAP-результате
	public static final int SLERR_SOAPR_ITEMREFNFOUND      = 423; // Не найден элемент SOAP-результата по ссылке '%s'
	public static final int SLERR_SOAPR_ITEMREFTYPECONFL   = 424; // Конфликт между атрибутами ref и type (%s) в элементе SOAP-ответа
	public static final int SLERR_SOPAR_UNRESITEMHASNTREF  = 425; // Неразрешенный элемент структуры (%s) SAOP-результата не имеет ссылки
	public static final int SLERR_SOAPR_UNRESOLVEDITEM     = 426; // Не удалось резрешить элемент структуры (%s) SOAP-результата
	public static final int SLERR_SOAPR_ITEMNAMENFOUND     = 427; // Элемент SOAP-пакета '%s' не найден
	public static final int SLERR_SOAPR_INVITEMPOS         = 428; // Недопустимый индекс (%s) элемента SOAP-пакета
	public static final int SLERR_WTMTA_INVSIGNATURE       = 429; // Неверная сигнатура WT-файла '%s'
	public static final int SLERR_USB_HIDUSBCLASSFAILED	   = 430; // Устройство не относится к классу USB или HID
	public static final int SLERR_SFILRDNULLOUTP           = 431; // Попытка чтения из NullOutput-файла
	public static final int SLERR_MAIL_NOTCONNECTED        = 432; // Внутренняя ошибка - не установлено соединение почтового клиента
	public static final int SLERR_MAIL_INVPROTOCOL         = 433; // Внутренняя ошибка - недопустимый протокол '%s' почтового клиента
	public static final int SLERR_MAIL_SMTP_NOREPLY        = 434; // Почтовый сервер SMTP не ответил на запрос.
	public static final int SLERR_MAIL_SMTP_REPLYERR       = 435; // Почтовый сервер SMTP вернул ошибку %s.
	public static final int SLERR_MAIL_POP3_NOREPLY        = 436; // Почтовый сервер POP3 не ответил на запрос.
	public static final int SLERR_MAIL_POP3_REPLYERR       = 437; // Почтовый сервер POP3 вернул ошибку '%s'
	public static final int SLERR_MAIL_POP3_UNDEFREPLY     = 438; // Почтовый сервер POP3 вернул не известный ответ '%s'
	public static final int SLERR_FTP_NOTCONNECTED         = 451; // FTP-соединение не установлено

	public static final int SLERR_PUNYCODE_BADINPUT        = 452; // PUNYCODE Input is invalid
	public static final int SLERR_PUNYCODE_BIGOUTPUT       = 453; // PUNYCODE Output would exceed the space provided
	public static final int SLERR_PUNYCODE_OVERFLOW        = 454; // PUNYCODE Wider integers needed to process input
	public static final int SLERR_CURL                     = 455; // Ошибка библиотеки libcurl
	public static final int SLERR_LIBXML                   = 456; // Ошибка библиотеки libxml
	public static final int SLERR_DUPSYMBWITHUNEQID        = 457; // Дублирование символа '%s' элемента с отличным идентификатором
	public static final int SLERR_INVIMAGESIZE             = 458; // Недопустимый размер изображения ([1..30000], [1..30000])
	public static final int SLERR_WTM_DUPLAYOUTSYMB        = 459; // @v10.4.9 Дублирование символа layout

	public static final int SLERR_FANN_CANT_OPEN_CONFIG_R          = 500; // Unable to open configuration file for reading
	public static final int SLERR_FANN_CANT_OPEN_CONFIG_W          = 501; // Unable to open configuration file for writing
	public static final int SLERR_FANN_WRONG_CONFIG_VERSION        = 502; // Wrong version of configuration file
	public static final int SLERR_FANN_CANT_READ_CONFIG            = 503; // Error reading info from configuration file
	public static final int SLERR_FANN_CANT_READ_NEURON            = 504; // Error reading neuron info from configuration file
	public static final int SLERR_FANN_CANT_READ_CONNECTIONS       = 505; // Error reading connections from configuration file
	public static final int SLERR_FANN_WRONG_NUM_CONNECTIONS       = 506; // Number of connections not equal to the number expected
	public static final int SLERR_FANN_CANT_OPEN_TD_W              = 507; // Unable to open train data file for writing
	public static final int SLERR_FANN_CANT_OPEN_TD_R              = 508; // Unable to open train data file for reading
	public static final int SLERR_FANN_CANT_READ_TD                = 509; // Error reading training data from file
	public static final int SLERR_FANN_CANT_ALLOCATE_MEM           = 510; // Unable to allocate memory
	public static final int SLERR_FANN_CANT_TRAIN_ACTIVATION       = 511; // Unable to train with the selected activation function
	public static final int SLERR_FANN_CANT_USE_ACTIVATION         = 512; // Unable to use the selected activation function
	public static final int SLERR_FANN_TRAIN_DATA_MISMATCH         = 513; // Irreconcilable differences between two <Fann::TrainData> structures
	public static final int SLERR_FANN_CANT_USE_TRAIN_ALG          = 514; // Unable to use the selected training algorithm
	public static final int SLERR_FANN_TRAIN_DATA_SUBSET           = 515; // Trying to take subset which is not within the training set
	public static final int SLERR_FANN_INDEX_OUT_OF_BOUND          = 516; // Index is out of bound
	public static final int SLERR_FANN_SCALE_NOT_PRESENT           = 517; // Scaling parameters not present
	public static final int SLERR_FANN_INPUT_NO_MATCH              = 518; // The number of input neurons in the ann and data don't match
	public static final int SLERR_FANN_OUTPUT_NO_MATCH             = 519; // The number of output neurons in the ann and data don't match
	public static final int SLERR_FANN_WRONG_PARAMETERS_FOR_CREATE = 520; // The parameters for create_standard are wrong, either too few parameters provided or a negative/very high value provided
	public static final int SLERR_FANN_INVLAYERCOUNT               = 521; // Недопустимое количество слоев нейронной сети
	public static final int SLERR_FANN_INVLAYERSIZE                = 522; // Недопустимый размер слоя нейронной сети
	public static final int SLERR_FANN_INVTRAINALG                 = 523; // Недопустимый алгоритм обучения нейронной сети
	//public static final int SLERR_JSON_OK = 1;                  // everything went smoothly
	//public static final int SLERR_JSON_MEMORY;                  // an error occurred when allocating memory
	public static final int SLERR_JSON_INCOMPLETE_DOCUMENT         = 550; // the parsed document didn't ended
	public static final int SLERR_JSON_WAITING_FOR_EOF             = 551; // A complete JSON document tree was already finished but needs to get to EOF. Other characters beyond whitespaces produce errors
	public static final int SLERR_JSON_MALFORMED_DOCUMENT          = 552; // the JSON document which was fed to this parser is malformed
	public static final int SLERR_JSON_INCOMPATIBLE_TYPE           = 553; // the currently parsed type does not belong here
	public static final int SLERR_JSON_ILLEGAL_CHARACTER           = 554; // the currently parsed character does not belong here
	public static final int SLERR_JSON_BAD_TREE_STRUCTURE          = 555; // the document tree structure is malformed
	public static final int SLERR_JSON_MAXIMUM_LENGTH              = 556; // the parsed string reached the maximum allowed size
	public static final int SLERR_JSON_UNKNOWN_PROBLEM             = 557; // some random, unaccounted problem occurred
	public static final int SLERR_URI_SYNTAX                       = 570; // Parsed text violates expected format
	public static final int SLERR_URI_NULL                         = 571; // One of the params passed was NULL although it mustn't be
	//public static final int SLERR_URI_MALLOC                     = 572; // Requested memory could not be allocated
	public static final int SLERR_URI_OUTPUT_TOO_LARGE             = 573; // Some output is to large for the receiving buffer
	public static final int SLERR_URI_NOT_IMPLEMENTED              = 574; // The called function is not implemented yet
	public static final int SLERR_URI_RANGE_INVALID                = 575; // The parameters passed contained invalid ranges
	// Errors specific to ToStr
	public static final int SLERR_URI_TOSTRING_TOO_LONG        = SLERR_URI_OUTPUT_TOO_LARGE; // Deprecated, test for URI_ERROR_OUTPUT_TOO_LARGE instead
	// Errors specific to AddBaseUri
	public static final int SLERR_URI_ADDBASE_REL_BASE             = 576; // Given base is not absolute
	// Errors specific to RemoveBaseUri
	public static final int SLERR_URI_REMOVEBASE_REL_BASE          = 577; // Given base is not absolute
	public static final int SLERR_URI_REMOVEBASE_REL_SOURCE        = 578; // Given base is not absolute

	//public static final int SLERR_ZIP_OK             0  // N No error
	public static final int SLERR_ZIP_FIRSTERROR                   = 591;  // @anchor
	public static final int SLERR_ZIP_MULTIDISK                    = 591;  // N Multi-disk zip archives not supported
	public static final int SLERR_ZIP_RENAME                       = 592;  // S Renaming temporary file failed
	public static final int SLERR_ZIP_CLOSE                        = 593;  // S Closing zip archive failed
	public static final int SLERR_ZIP_SEEK                         = 594;  // S Seek error
	public static final int SLERR_ZIP_READ                         = 595;  // S Read error
	public static final int SLERR_ZIP_WRITE                        = 596;  // S Write error
	public static final int SLERR_ZIP_CRC                          = 597;  // N CRC error
	public static final int SLERR_ZIP_ZIPCLOSED                    = 598;  // N Containing zip archive was closed
	public static final int SLERR_ZIP_NOENT                        = 599;  // N No such file
	public static final int SLERR_ZIP_EXISTS                       = 600;  // N File already exists
	public static final int SLERR_ZIP_OPEN                         = 601;  // S Can't open file
	public static final int SLERR_ZIP_TMPOPEN                      = 602;  // S Failure to create temporary file
	public static final int SLERR_ZIP_ZLIB                         = 603;  // Z Zlib error
	public static final int SLERR_ZIP_MEMORY                       = 604;  // N Malloc failure
	public static final int SLERR_ZIP_CHANGED                      = 605;  // N Entry has been changed
	public static final int SLERR_ZIP_COMPNOTSUPP                  = 606;  // N Compression method not supported
	public static final int SLERR_ZIP_EOF                          = 607;  // N Premature end of file
	public static final int SLERR_ZIP_INVAL                        = 608;  // N Invalid argument
	public static final int SLERR_ZIP_NOZIP                        = 609;  // N Not a zip archive
	public static final int SLERR_ZIP_INTERNAL                     = 610;  // N Internal error
	public static final int SLERR_ZIP_INCONS                       = 611;  // N Zip archive inconsistent
	public static final int SLERR_ZIP_REMOVE                       = 612;  // S Can't remove file
	public static final int SLERR_ZIP_DELETED                      = 613;  // N Entry has been deleted
	public static final int SLERR_ZIP_ENCRNOTSUPP                  = 614;  // N Encryption method not supported
	public static final int SLERR_ZIP_RDONLY                       = 615;  // N Read-only archive
	public static final int SLERR_ZIP_NOPASSWD                     = 616;  // N No password provided
	public static final int SLERR_ZIP_WRONGPASSWD                  = 617;  // N Wrong password provided
	public static final int SLERR_ZIP_OPNOTSUPP                    = 618;  // N Operation not supported
	public static final int SLERR_ZIP_INUSE                        = 619;  // N Resource still in use
	public static final int SLERR_ZIP_TELL                         = 620;  // S Tell error
	public static final int SLERR_ZIP_COMPRESSED_DATA              = 621;  // @v10.8.4 N Compressed data invalid
	public static final int SLERR_ZIP_CANCELLED                    = 622;  // @v10.8.4 N Operation cancelled
	public static final int SLERR_ZIP_LASTERROR                    = 622;  // @anchor @v10.8.4 620-->622
	public static final int SLERR_ZLIB_BUFINFLATEFAULT             = 623;  // @v11.2.9 Ошибка распаковки буфера данных

	public static final int SLERR_CRYPTO_INVPARAM                  = 651;  // @v11.0.11 Ошибка SLCRYPTO - недопустимый параметр функции
	public static final int SLERR_CRYPTO_KEYBUFOVRFLW              = 652;  // @v11.0.11 Ошибка SLCRYPTO - переполнение внутреннего буфера ключа
	public static final int SLERR_CRYPTO_INVKEYSIZE                = 653;  // @v11.0.11 Ошибка SLCRYPTO - недопустимая длина ключа шифрования
	public static final int SLERR_CRYPTO_INVIVSIZE                 = 654;  // @v11.0.11 Ошибка SLCRYPTO - недопустимая длина IV
	public static final int SLERR_CRYPTO_OPENSSL                   = 655;  // @v11.0.11 Ошибка OpenSSL
	// Коды с префиксом SLERR_AMQP_ являются трансляцией кодов состояний библиотеки rabbitmq-c
	public static final int SLERR_AMQP_UNKN                        = 700; // Неизвестный AMQP-статус
	public static final int SLERR_AMQP_BAD_AMQP_DATA               = 701;
	public static final int SLERR_AMQP_UNKNOWN_CLASS               = 702;
	public static final int SLERR_AMQP_UNKNOWN_METHOD              = 703;
	public static final int SLERR_AMQP_HOSTNAME_RESOLUTION_FAILED  = 704;
	public static final int SLERR_AMQP_INCOMPATIBLE_AMQP_VERSION   = 705;
	public static final int SLERR_AMQP_CONNECTION_CLOSED           = 706;
	public static final int SLERR_AMQP_BAD_URL                     = 707;
	public static final int SLERR_AMQP_SOCKET                      = 708;
	public static final int SLERR_AMQP_INVALID_PARAMETER           = 709;
	public static final int SLERR_AMQP_TABLE_TOO_BIG               = 710;
	public static final int SLERR_AMQP_WRONG_METHOD                = 711;
	public static final int SLERR_AMQP_TIMEOUT                     = 712;
	public static final int SLERR_AMQP_TIMER_FAILURE               = 713;
	public static final int SLERR_AMQP_HEARTBEAT_TIMEOUT           = 714;
	public static final int SLERR_AMQP_UNEXPECTED_STATE            = 715;
	public static final int SLERR_AMQP_SOCKET_CLOSED               = 716;
	public static final int SLERR_AMQP_SOCKET_INUSE                = 717;
	public static final int SLERR_AMQP_BROKER_UNSUPP_SASL_METHOD   = 718;
	public static final int SLERR_AMQP_UNSUPPORTED                 = 719;
	public static final int SLERR_AMQP_TCP                         = 720;
	public static final int SLERR_AMQP_TCP_SOCKETLIB_INIT          = 721;
	public static final int SLERR_AMQP_SSL                         = 722;
	public static final int SLERR_AMQP_SSL_HOSTNAME_VERIFY_FAILED  = 723;
	public static final int SLERR_AMQP_SSL_PEER_VERIFY_FAILED      = 724;
	public static final int SLERR_AMQP_SSL_CONNECTION_FAILED       = 725;
	//
	// Типы объектов данных. Заимствованы из ppdefs.h
	//
	public static final int PPOBJ_CONFIG           =   1;  // $(PPSecur) Конфигурация системы
	public static final int PPOBJ_STATUS           =   2;  // @unused Текущее состояние
	public static final int PPOBJ_USRGRP           =   3;  // $(PPSecur) Группы пользователей (struct PPSecur)
	public static final int PPOBJ_USR              =   4;  // $(PPSecur) Пользователи (struct PPSecur)
	public static final int PPOBJ_UNIT             =   5;  // $ Единицы измерения (struct PPUnit)
	public static final int PPOBJ_CURRATE          =   6;  // @unused # Просто строки
	public static final int PPOBJ_CITYSTATUS       =   7;  // $(PPWorldObjStatus) Статусы географических объектов
	public static final int PPOBJ_PERSONKIND       =   8;  // $ Виды персоналий
	public static final int PPOBJ_PRSNSTATUS       =   9;  // $ Юридические статусы персоналий
	public static final int PPOBJ_STAFFRANK        =  10;  // #
	public static final int PPOBJ_OPRTYPE          =  11;  // $ Типы операций *
	public static final int PPOBJ_OPRKIND          =  12;  // $ Виды операций
	public static final int PPOBJ_ACCSHEET         =  13;  // $ Таблицы аналитических статей
	public static final int PPOBJ_BNKACCTYPE       =  14;  // # Типы банковских счетов *
	public static final int PPOBJ_PRICETYPE        =  15;  // @unused Типы цен
	public static final int PPOBJ_AMOUNTTYPE       =  16;  // $ Типы сумм документов *
	public static final int PPOBJ_CASHNODE         =  17;  // $ Расчетные кассовые узлы
	public static final int PPOBJ_ACTION           =  18;  // # События, регистрируемые в системном журнале *
	//
	// Фантомный объект. В таблице Reference для него не существует записей.
	// Могут существовать записи в таблице Property, реализующие линейный
	// список групп операций по счетчику. При изменении счетчика любой из
	// операций, принадлежащих одной группе, значение счетчика в остальных
	// операциях группы изменяется синхронно.
	//
	public static final int PPOBJ_COUNTGRP         =  19;  // @unused Использовался до версии 3.0.1
	public static final int PPOBJ_BCODESTRUC       =  20;  // $ Система внутренних штрих-кодов
	public static final int PPOBJ_DBDIV            =  21;  // $ Разделы базы данных
	public static final int PPOBJ_GOODSTYPE        =  22;  // $ Типы товаров
	//
	// @v1.9.10
	// Виртуальный объект. Представлен набором типов проблем.
	// В принципе записи в таблице Reference для него существовать
	// могут, но любой тип проблемы жестко зашит в программу.
	//
	public static final int PPOBJ_PROBLEM          =  23; // @unused
	public static final int PPOBJ_GOODSSTRUC       =  24; // $ Структуры товаров
	public static final int PPOBJ_FORMULA          =  25; // # Формулы
	public static final int PPOBJ_SCALE            =  26; // $ Электронные весы
	public static final int PPOBJ_REGISTERTYPE     =  27; // $ Типы регистрационных документов
	public static final int PPOBJ_ELINKKIND        =  28; // $ Виды адресов электронной связи
	public static final int PPOBJ_QUOTKIND         =  29; // $ Виды котировок
	public static final int PPOBJ_PERSONOPKIND     =  30; // $ Вид персональной операции
	public static final int PPOBJ_TAG              =  31; // $ Теги объектов
	public static final int PPOBJ_GOODSTAX         =  32; // $ Налоговые группы товаров
	public static final int PPOBJ_BCODEPRINTER     =  33; // $ Принтеры штрихкодов
	public static final int PPOBJ_CURRENCY         =  34; // Валюты
	public static final int PPOBJ_CURRATETYPE      =  35; // Типы валютных курсов
	public static final int PPOBJ_OPCOUNTER        =  36; // Счетчики документов
	public static final int PPOBJ_GOODSCLASS       =  37; // Отраслевые классы товаров
	public static final int PPOBJ_GOODSBASKET      =  40; // $ Корзина товаров
	public static final int PPOBJ_BHT              =  41; // $ BHT-терминалы
	public static final int PPOBJ_SCARDSERIES      =  42; // $ Серия персональных карт
	public static final int PPOBJ_STYLOPALM        =  43; // $ PDA Palm
	public static final int PPOBJ_ASSTWROFFGRP     =  44; // $ Группа списания активов
	public static final int PPOBJ_DRAFTWROFF       =  45; // $ Конфигурация списания драфт-документов
	public static final int PPOBJ_ADVBILLKIND      =  46; // $ Виды первичных документов (например, для авансовых отчетов)
	public static final int PPOBJ_TRANSPMODEL      =  47; // # Модели транспортных средств
	public static final int PPOBJ_INTERNETACCOUNT  =  48; // $ Почтовые учетные записи
	public static final int PPOBJ_PERSONRELTYPE    =  49; // $ Тип отношения между персоналиями
	public static final int PPOBJ_PRSNCATEGORY     =  50; // # Категории персоналий
	public static final int PPOBJ_BILLSTATUS       =  51; // $ Статусы документов
	public static final int PPOBJ_TOUCHSCREEN      =  52; // $ TouchScreen
	public static final int PPOBJ_DUTYSCHED        =  53; // $ Расписания дежурств
	public static final int PPOBJ_DATETIMEREP      =  54; // $ Периодичность
	public static final int PPOBJ_LOCPRINTER       =  55; // $ Принтеры, привязанные к складам
	public static final int PPOBJ_SALCHARGE        =  56; // $ Схема начисления зарплаты
	public static final int PPOBJ_STAFFCAL         =  58; // $ Штатный календарь
	public static final int PPOBJ_NAMEDOBJASSOC    =  59; // $ Именованные ассоциации объектов
	public static final int PPOBJ_BIZSCORE         =  60; // Определители бизнес-показателей
	public static final int PPOBJ_GLOBALUSERACC    =  61; // Учетная запись глобального пользователя (внешнего по отношению к текущей базе данных)
	public static final int PPOBJ_GOODSVALRESTR    =  62; // Ограничения товарных величин
	public static final int PPOBJ_PALLET           =  63; // Складские паллеты
	public static final int PPOBJ_DEBTDIM          =  64; // Размерность расчета долгов по документам
	public static final int PPOBJ_EVENTTOKEN       =  65; // Специальный объект, используемый как уникальная метка для событий.
	public static final int PPOBJ_SMSPRVACCOUNT    =  66; // Учетная запись провайдера рассылки SMS
	public static final int PPOBJ_PHONESERVICE     =  67; // Оборудование компьютерной телефонии
	public static final int PPOBJ_GTACTION         =  68; // # События, регистрируемые в журнале тарфицируемых операций глобальных аккаунтов
	public static final int PPOBJ_SENDSMS          =  69; // Отправка смс
	public static final int PPOBJ_UHTTSTORE        =  70; // Интернет-магазин Universe-HTT
	public static final int PPOBJ_GENERICDEVICE    =  71; // Обобщенное устройство
	public static final int PPOBJ_WORKBOOK_PRE813  =  72; // элемент сайта
	//
	// Функциональные единицы, являющиеся объектами замера производительности. Все значения являются зарезервированными
	// и их идентификаторы не могут быть изменены.
	//
	public static final int PPOBJ_USERPROFILE      =  73; // @v8.0.6
	public static final int PPOBJ_EDIPROVIDER      =  74; // Провайдеры EDI-обмена
	public static final int PPOBJ_USREXCLRIGHTS    =  75; // Исключения для пользовательских прав доступа при некоторых действиях
	public static final int PPOBJ_STAFFLIST2       =  76; // @v9.0.3 Штатное расписание (2-я версия объекта - записи перемещены из
	public static final int PPOBJ_ACCOUNT2         =  77; // Бухгалтерские счета (балансовые) (2-я версия объекта - записи перемещены из
	public static final int PPOBJ_TIMESERIES       =  78; // @v10.2.3 Временные серии. Специализированный объект, служащий в качестве заголовков временных серий данных.
	public static final int PPOBJ_FREIGHTPACKAGETYPE = 79; // @v10.4.1 Тип транспортной упаковки
	public static final int PPOBJ_TAXSYSTEMKIND      = 80; // @v10.6.0 Вид системы налогообложения //
	public static final int PPOBJ_TSSMODEL           = 81; // @v10.7.4 Параметры модели расчета стратегий на временных сериях
	public static final int PPOBJ_EVENTSUBSCRIPTION  = 82; // @v10.8.9 Подписка на события //
	public static final int PPOBJ_DYNAMICOBJS    =   100; // Список динамических объектов
	public static final int PPOBJ_UNASSIGNED     =  1000;  // Идентификатор объекта имеет специальное назначение и не соответствует какому-либо реальному объекту.
	public static final int PPOBJ_PERSON         =  1004;  // Персоналии
	public static final int PPOBJ_ARTICLE        =  1006;  // Статьи аналитического учета
	public static final int PPOBJ_GOODSGROUP     =  1008;  // Группы товаров
	public static final int PPOBJ_GOODS          =  1009;  // Товары
	public static final int PPOBJ_LOCATION       =  1010;  // Позиции @store(LocationTbl)
	public static final int PPOBJ_BILL           =  1011;  // Документы @store(BillTbl)
	public static final int PPOBJ_ACCTURN        =  1012;  // Бухгалтерские проводки
	public static final int PPOBJ_QCERT          =  1013;  // Сертификаты качества
	public static final int PPOBJ_PRICELIST      =  1014;  // Прайс-листы
	public static final int PPOBJ_VATBOOK        =  1015;  // Книги продаж и покупок
	public static final int PPOBJ_REGISTER       =  1017;  // Регистрационные документы
	public static final int PPOBJ_PERSONEVENT    =  1018;  // Персональные события //
	public static final int PPOBJ_INVENTORY      =  1019;  // Инвентаризация //
	public static final int PPOBJ_LOT            =  1022;  // Лоты
	public static final int PPOBJ_CSESSION       =  1023;  // Кассовые сессии
	public static final int PPOBJ_ACCTREL        =  1024;  // !PPObject Бухгалтерские связки
	public static final int PPOBJ_OBJASSOC       =  1025;  // !PPObject Ассоциации объектов
	public static final int PPOBJ_PACKAGE        =  1026;  // !PPObject Товарные пакеты
	public static final int PPOBJ_PCKGTYPE       =  1027;  // Types of goods packages
	public static final int PPOBJ_TRANSPORT      =  1028;  // Транспортные средства
	public static final int PPOBJ_QUOT           =  1029;  // !PPObject Котировки
	public static final int PPOBJ_GOODSREST      =  1030;  // !PPObject Остатки товаров (синхронизируется)
	public static final int PPOBJ_SCARD          =  1031;  // Персональные карты
	public static final int PPOBJ_CCHECK         =  1032;  // !PPObject Кассовые чеки
	public static final int PPOBJ_PROJECT        =  1033;  // Проекты  @store(ProjectTbl)
	public static final int PPOBJ_BRAND          =  1034;  // Брэнды   @store(Goods2Tbl)
	public static final int PPOBJ_MRPTAB         =  1035;  // MRP-таблицы
	public static final int PPOBJ_PRJTASK        =  1036;  // Задачи по проекту
	public static final int PPOBJ_PROCESSOR      =  1037;  // Процессоры
	public static final int PPOBJ_TECH           =  1038;  // Технологии
	public static final int PPOBJ_TSESSION       =  1039;  // Процессорные сессии
	public static final int PPOBJ_DFCREATERULE   =  1040; // Правила для создания драфт документа
	public static final int PPOBJ_BILLSYMB       =  1041;  // !PPObject Символы формул сумм документов (используется только для организации кэша)
	public static final int PPOBJ_DBXCHG         =  1042;  // !PPObject Обмен данными между разделами (только для блокировки приема разными пользователями)
	public static final int PPOBJ_WORLD          =  1043;  // Географические объекты
	public static final int PPOBJ_GOODSINFO      =  1044;  //
	public static final int PPOBJ_PERSONPOST     =  1045;  //  Должностные назначения //
	public static final int PPOBJ_DESKTOP        =  1046;  // !PPObject Рабочий стол. Нужен для сохранения логотипа рабочего стола и некоторых свойств в PropertyTbl
	public static final int PPOBJ_SERIAL         =  1047;  // !PPObject Ассоциации серийных номеров с уникальными идентификаторами.
	// Используется для хранения привязки серийных номеров к записям группировка строк кассовых чеков (CGoodsLine)
	// @store(ObjTagTbl)
	public static final int PPOBJ_TSALESBUILD    =  1048;  // !PPObject Построение таблицы продаж 
	public static final int PPOBJ_DL600DATA      =  1049;  // !PPObject Структура данных DL600
	public static final int PPOBJ_QUOT2REL       =  1050;  // !PPObject Группирующие характеристики второй реализации котировок
	public static final int PPOBJ_QUOT2          =  1051;  // !PPObject
	public static final int PPOBJ_GOODSARCODE    =  1052;  // !PPObject
	public static final int PPOBJ_BILLEXT        =  1053;  // !PPObject Используется для кэширования расширения документов
	public static final int PPOBJ_BUDGET         =  1054;  // Бюджеты
	public static final int PPOBJ_BIZSCTEMPL     =  1055;  // Шаблоны бизнес показателей
	public static final int PPOBJ_CAFETABLE      =  1056;  // Столы HORECA // Since @v8.4.1 Real Object
	public static final int PPOBJ_RFIDDEVICE     =  1057;  // PPObject rfid устройства
	public static final int PPOBJ_SPECSERIES     =  1058;  //
	public static final int PPOBJ_EADDR          =  1059;  //
	public static final int PPOBJ_GTA            =  1060;  // !PPObject Специальный тип для обозначения тарифицируемых событий глобальных учетных записей.
	public static final int PPOBJ_CURRATEIDENT   =  1061;  // !PPObject @uhtt Значение курса валюты
	public static final int PPOBJ_UHTTSCARDOP    =  1062;  // !PPObject @uhtt Используется для отображения информации о карте
	public static final int PPOBJ_TAGVALUE       =  1063;  // !PPObject Специальный тип для идентификации значений тегов объектов
	public static final int PPOBJ_CHKINP         =  1064;  // !PPObject Специальный тип для персональных регистраций
	public static final int PPOBJ_COMPGOODS      =  1065;  // Товары с составляющими // @vmiller
	public static final int PPOBJ_WORKBOOK       =  1066;  // рабочие книги (замещает PPOBJ_WORKBOOK_PRE813)
	public static final int PPOBJ_BILLFREIGHT    =  1068;  // !PPObject Используется для кэширования фрахтов документов
	public static final int PPOBJ_FIAS           =  1069;  // Объект, управляющий данными справочника адресов России ФИАС. Не является полноправным PPObject'ом
	public static final int PPOBJ_UUIDREF        =  1070;  // !PPObject Специальное значение для идентификации данных таблицы UuidRef
	public static final int PPOBJ_EGAISREFA      =  1071;  // Специализированный объект, хранящий данные справок А системы ЕГАИС
	public static final int PPOBJ_EGAISPRODUCT   =  1072;  // Специализированный объект, хранящий данные о товарах системы ЕГАИС
	public static final int PPOBJ_EGAISPERSON    =  1073;  // Специализированный объект, хранящий данные о персоналиях системы ЕГАИС
	public static final int PPOBJ_VETISENTITY    =  1074;  // @v10.0.09 Специализированный объект, хранящий данные о сущностях системы VETIS
	public static final int PPOBJ_GEOTRACKING    =  1075;  // @v10.1.5 !PPObject Специализированный объект для представления гео-треков.
		// Значение фактически применяется только для идентификации запроса к JobServer'у.
	public static final int PPOBJ_EVENT          =  1076;  // @v10.8.9 События
	public static final int PPOBJ_STYLOQBINDERY  =  1077;  // @v11.1.7 Подшивка объектов Stylo-Q
	public static final int PPOBJ_DBCONVERT      =  1100;  // Специальный объект, используемый для формирования записей
	// с информацией о конвертации базы данных. Запись, идентифицирующая конвертацию имеет идентификатор
	// вида 0xMJMNRR  где MJ - мажор, MN - минор, RR - релиз
	// Например, после выполнения конвертации 6.4.06 появится запись с ИД 0x060406
	public static final int PPOBJ_ADVEVENTQUEUE  =  1101;   // Специальное значение для идентификации очереди системных событий
	public static final int PPOBJ_SELFREFTEXT    =  2000;   // Специальное значение для записей в TextRef, представлющих только текст с автоматически назначенным идентификатором
	//
	// @v11.3.0 Общие (и, иногда, частные) атрибуты объектов. Вводятся для управления техникой поиска данных по текстовым запросам (fulltext index).
	// Attention! Чтобы минимизировать разногласия между методами идентификации многие атрибуты имеют значения, совпадающие с константами PPTRPROP_XXX (pp.h)
	// Если добавляете новые значения, то согласуйте их с соответствующим набором констант.
	// @persistent
	//
	public static final int PPOBJATTR_NAME      =   1; // Наименование
	public static final int PPOBJATTR_SYMB      =   2; // Символ
	public static final int PPOBJATTR_LONGNAME  =   3; // Длинное наименование
	public static final int PPOBJATTR_MEMO      =   4;
	public static final int PPOBJATTR_RAWADDR   =   6;
	public static final int PPOBJATTR_DESCR     =   8;
	public static final int PPOBJATTR_CODE      =   9;
	public static final int PPOBJATTR_ID        =  21; // Целочисленный уникальный идентификатор
	public static final int PPOBJATTR_FLAGS     =  22;
	public static final int PPOBJATTR_PHONE     =  23;
	public static final int PPOBJATTR_EMAIL     =  24;
	public static final int PPOBJATTR_GLN       =  25;
	public static final int PPOBJATTR_RUINN     =  26;
	public static final int PPOBJATTR_RUKPP     =  27;
	public static final int PPOBJATTR_RUSNILS   =  28;
	//
	// Descr: Идентификаторы системных событий. Заимствованы из ppdefs.h
	// В stylo-q используется лишь небольшая часть этих значений (тем не менее, заимствование полное
	// и подлежит актуализации при изменении оригинала).
	// Текстовые строки, соответствующие событиям содержатся в группе [PPSTR_ACTION=112]
	// со значениями, равными приведенным ниже символам.
	// Символы и значения констант синхронизированы с проектом Papyrus (see ppdefs.h)
	//
	public static final int PPACN_LOGIN                  = 1;
	public static final int PPACN_LOGOUT                 = 2;
	public static final int PPACN_TURNBILL               = 3;
	public static final int PPACN_UPDBILL                = 4;
	public static final int PPACN_RMVBILL                = 5;
	public static final int PPACN_PRNBILL                = 6;
	public static final int PPACN_ACCESSCASH             = 7;
	public static final int PPACN_CSESSCLOSED            = 8; // Закрытие кассовой сессии
	public static final int PPACN_EXPCASHSESS            = 9; // Выгрузка данных для Async Cash Sess
	public static final int PPACN_UPDCASHSESS           = 10; //
	public static final int PPACN_OBJADD                = 11; // Объект добавлен
	public static final int PPACN_OBJUPD                = 12; // Объект изменен
	public static final int PPACN_OBJRMV                = 13; // Объект удален
	public static final int PPACN_OBJUNIFY              = 14; // Объекты объединены
	public static final int PPACN_TRANSMOD              = 15; // Переданы изменения (PPOBJ_DBDIV, DestDbDivID)
	public static final int PPACN_GOODSQUOTUPD          = 16; // Изменены котировки товара
	public static final int PPACN_SCARDDISUPD           = 17; // Изменена скидка по дисконтной карте
	public static final int PPACN_GOODSNODISRMVD        = 18; // Снят признак "Без скидки" с товара
	public static final int PPACN_GOODSVATGRPCHD        = 19; // Изменена налоговая группа на товар
	public static final int PPACN_SCARDBINDUPD          = 20; // Изменена привязка карты.
	// Объект: документ или чек, Extra - ид предшествующей карты.
	public static final int PPACN_MAINTAINPRJTASK       = 21; // Осуществлена процедура обслуживания задач
	// Extra - количество измененных задач
	public static final int PPACN_BILLSTATUSUPD         = 22; // Изменен статус документа. Дополнительному полю
	// присваивается значение предшествующего статуса
	public static final int PPACN_CONFIGCREATED         = 23; // Создана конфигурация //
	public static final int PPACN_CONFIGUPDATED         = 24; // Изменена конфигурация //
	public static final int PPACN_OBJVIEWED             = 25; // Объект просмотрен
	public static final int PPACN_TRANSMIT              = 26; // Осуществлена передача данных в другой раздел
	public static final int PPACN_CLEARCHECK            = 27; // Очистка чека
	public static final int PPACN_RMVCHKLINE            = 28; // Удаление позиции из чека
	public static final int PPACN_RMVALLGDSALTGRP       = 29; // Удаление всех товаров из альт.группы
	public static final int PPACN_AUTOFILLALTGRP        = 30; // Автозаполнение альт.группы
	public static final int PPACN_CHNGPLUALTGRP         = 31; // Изменение номера PLU в альт.группе
	public static final int PPACN_GOODSMTXUPD           = 32; // Изменена котировка товарной матрицы. В качестве объекта
	// указывается пара {PPOBJ_GOODS, goods_id}, в качестве дополнительного параметра - ид склада, к
	// которому относится элемент измененной матрицы. Если доп параметр == -1, то склад не определен,
	// если доп параметр == 0, то речь идет о котировке по всем складам
	public static final int PPACN_CCHECKUPDATED         = 33; // Некоторые параметры чека изменены в ручную
	public static final int PPACN_OBJSYNCACCEPTED       = 34; // @construction Объекта акцептирован в другом разделе БД.
	// Дополнительный параметр является идентификатором раздела, в котором акцептирован объект.
	// Дата и время события соответствуют дате и времени акцепта объекта в другом разделе.
	// Из-за этого данное событие вводится не стандартной функцией
	public static final int PPACN_OBJSYNCCOMMIT         = 35; // Фиксирована транзакция приема данных из других разделов.
	public static final int PPACN_BILLCORRECTED         = 36; // Документ исправлен функцией восстановления //
	public static final int PPACN_EXT_BILLCORRECTED_LINKRESET = 1; // Обнулена ссылка на отсутствующий связанный документ
	public static final int PPACN_INVENTWROFF           = 37; // Списание инвентаризации
	public static final int PPACN_INVENTROLLBACK        = 38; // Откат списания инвентаризации
	public static final int PPACN_INVENTBYTERM          = 39; // Загрузка инвентаризации из терминала
	public static final int PPACN_INVENTUNITE           = 40; // Объединение инвентаризаций
	public static final int PPACN_OPCNTRUPD             = 41; // Счетчик по виду операции изменен
	// Дополнительное поле получает величину, равную разнице между новым и старым значением счетчика
	public static final int PPACN_GOODSMTXRESTRUPD      = 42; // Изменено ограничение на количество элементов матрицы. В качестве объекта
	// указывается пара {PPOBJ_GOODS, goods_id}, в качестве дополнительного параметра - ид склада, к
	// которому относится элемент измененной матрицы. Если доп параметр == -1, то склад не определен,
	// если доп параметр == 0, то речь идет о котировке по всем складам
	public static final int PPACN_ARSTOPSET             = 43; // На аналитическую статью установлен флаг STOP
	public static final int PPACN_ARSTOPRESET           = 44; // С аналитической статьи снят флаг STOP
	public static final int PPACN_BIZSCOREUPDATED       = 45; // Пересчитаны бизнес-показатели
	public static final int PPACN_GLOBBIZSCOREUPD       = 46; // Обновлены глобальные бизнес-показатели
	public static final int PPACN_BILLCCHKPRINTED       = 47; // По документу отпечатан кассовый чек
	// В качестве доп значения передается ИД кассового узла, по которому отпечатан чек.
	public static final int PPACN_OBJEXTMEMOUPD         = 48; // Изменено расширенное примечание по объекту (возможно, удалено)
	public static final int PPACN_UPDBILLEXT            = 49; // Изменено расширение документа (независимо от основной части документа)
	public static final int PPACN_CCHECKDELETED         = 50; // Чек удален (регистрируется только для ручных операций удаления одного чека).
	public static final int PPACN_OBJTAGUPD             = 51; // Изменен тег объекта. Доп параметр - ID тега.
	public static final int PPACN_INVENTTURN            = 52; // Создан документ инвентаризации
	public static final int PPACN_RCVRFREIGHTCITY       = 53; // Восстановлен город во фрахте документа
	public static final int PPACN_UPDBILLFREIGHT        = 54; // Изменен фрахт документа (независимо от основной части документа)
	public static final int PPACN_UPDBILLOBJECT         = 55; // Изменен контрагент по документу. Доп параметр - ID предыдущего значения контрагента
	public static final int PPACN_UPDTSESSPRC           = 56; // Изменен процессор в техн сессии. Доп параметр - ID предыдущего значения процессора
	public static final int PPACN_UNMODIFBILLTRANS      = 57; // Часть изменений принятого документа не могут быть акцептированы.
	// Доп параметр - ID раздела-отправителя //
	public static final int PPACN_MTXGOODSADD           = 58; // Создан товар, один из членов родительской ветки которого принадлежит
	// товарной матрице (по любому складу). Это событие необходимо для обновления кэша товарной матрицы другими
	// сеансами для адекватной вставки товара в матрицу.
	// Доп параметр: ID родительской группы, принадлежащей матрице //
	public static final int PPACN_CCHECKTOGGLEDLVR      = 59; // Изменен признак завершения доставки по чеку. Доп параметр: 1 - признак установлен, 0 - признак снят
	public static final int PPACN_UNDOCSESSWROFF        = 60; // Откат списания кассовой сессии
	public static final int PPACN_UNDOCSESSGRPNG        = 61; // Откат группировки кассовой сессии
	public static final int PPACN_SUPPLDEALUPD          = 62; // Изменена контрактная цена по поставщику. Доп параметр: ИД поставщика
	public static final int PPACN_QUOTUPD2              = 63; // Изменена котировка (2) товара. Доп параметр: ИД транзакции.
	public static final int PPACN_QUOTRMV2              = 64; // Удалена котировка (2) товара. Доп параметр: ИД транзакции.
	public static final int PPACN_DISPOSEBILL           = 65; // Товарный документ был размещен по складским ячейкам
	public static final int PPACN_EVENTTOKEN            = 66; // Событийная метка с объектом типа PPOBJ_EVENTTOKEN
	public static final int PPACN_UNITECCHECK           = 67; // Объединены два чека. ИД объекта - чек, который остался, доп параметр: ИД чека, который был влит в первый.
	public static final int PPACN_SCARDBONUSCHARGE      = 68; // По бонусной карте начислен бонус. Доп параметр: сжатое значение периода начисления.
	public static final int PPACN_OBJTAGRMV             = 69; // Удален тег объекта. Доп параметр - ID тега.
	public static final int PPACN_PERSONEVENTREDO       = 70; // Повтор персонального события //
	public static final int PPACN_SCARDOPMOV            = 71; // Операция по карте перемещена на другую карту. Доп параметр - ID карты назначения //
	public static final int PPACN_SMSSENDED             = 72; // Отправлено SMS персоналии
	public static final int PPACN_OBJTAGADD             = 73; // Создан тег объекта. Доп параметр - ID тега.
	public static final int PPACN_SCARDTRNOVRUPD        = 74; // Изменено значение оборота по персональной карте
	public static final int PPACN_UPDBILLWLABEL         = 75; // Изменен признак WhiteLabel по документу
	public static final int PPACN_SCARDACTIVATED        = 76; // Персональная карта активирована
	public static final int PPACN_STYLOREGISTERING      = 77; // Зарегистрировано устройство Stylo
	public static final int PPACN_ATTACHMENTUPD         = 78; // Приклепленный к объекту файл изменен (доп параметр - номер файла)
	public static final int PPACN_BILLWROFF             = 79; // Документ списан
	public static final int PPACN_BILLWROFFUNDO         = 80; // Откат списания документа
	public static final int PPACN_GOODSPASSVSET         = 81; // Установлен признак "Пассивный" для товара
	public static final int PPACN_GSTRUCATTACHED        = 82; // @v10.2.1 К товару привязана товарная структура
	public static final int PPACN_GSTRUCDETACHED        = 83; // @v10.2.1 От товара отвязана товарная структура
	public static final int PPACN_GSTRUCCUPD            = 84; // @v10.2.1 Изменился компонентный состав товарной структуры (добавлен или удален компонент)
	public static final int PPACN_TSSERIESUPD           = 85; // @v10.3.3 Изменена временная серия
	public static final int PPACN_TSSTRATEGYUPD         = 86; // @v10.3.3 Изменена стратегия временной серии
	public static final int PPACN_SCARDEXPRYUPD         = 87; // @v10.5.10 Изменена дата истечения срока действия персональной карты
	public static final int PPACN_SYSMAINTENANCE        = 88; // @v10.5.12 Событие вызова одной из специальных функций обслуживания системы. Дополнительный параметр - ид функции
	public static final int PPACN_BILLUNLINK            = 89; // @v10.8.3 Удалена привязка документа к другому документу. Дополнительный параметр - ид документа, к которому была привязка
	public static final int PPACN_EVENTDETECTION        = 90; // @v10.8.10 Попытка обнаружения события (PPObjEvent). Следующая попытка будет опираться на время этой записи.
	public static final int PPACN_RECENTVERSIONLAUNCHED = 91; // @v11.2.10 Событие, иниициируемое при запуске сеанса, версия которого превышает последнее значение, зафиксированное
		// таким же событием. Используетс в Stylo-Q и, возможно, в иных однопользовательских приложениях. В Papyrus'е не применяется из-за того, что он многопользовательский -
		// там другие механизмы.
	public static final int PPACN_STYLOQSVCSELFREG       = 92; // @v11.2.12 Сервис Stylo-Q отправил собственные регистрационные данные медиатору. Доп параметр - ид медиатора.
			// Событие используется для правильной идентификации необходимости повторной саморегистрации после изменения собственных данных.
	public static final int PPACN_SCARDOWNERSET          = 93; // @v11.3.1 Установлен владелец персональной карты. Доп параметр - ID предыдущего владельца.
	public static final int PPACN_STYLOQSVCIDXQUERY      = 94; // @v11.3.4 Сервис Stylo-Q отправил медиатору данные для индексации. Доп параметр - ид медиатора.
		// Событие используется для правильной идентификации необходимости повторной посылки данных.
	public static final int PPACN_STYLOQFACETRANSMITTED  = 95; // @v11.3.9 Клиент Stylo-Q передал сервису свой лик. Объект: oid сервиса {PPOBJ_STYLOQBINDERY; id},
	// доп параметр - ид лика. Событие используется для правильной идентификации необходимости повторной посылки данных.
	// !!! При вставке нового значения изменить PPACN_LAST !!!
	public static final int PPACN_LAST                   = 95; // Последнее значение (ИЗМЕНИТЬ ПРИ ВСТАВКЕ НОВОГО ЗНАЧЕНИЯ)


	public static boolean ishex(char c) { return ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')); }
	public static boolean ishex(byte c) { return ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')); }
	public static boolean isdec(char c) { return (c >= '0' && c <= '9'); }
	public static boolean isdec(byte c) { return (c >= '0' && c <= '9'); }
	public static int hex(byte c) { return (c >= '0' && c <= '9') ? (c-'0') : ((c >= 'A' && c <= 'F') ? (c-'A'+10) : ((c >= 'a' && c <= 'f') ? (c-'a'+10) : 0)); }
	public static boolean isasciialpha(byte c) { return (((c | 32) - 97) < 26); }

	public static boolean isletter866(byte c)
	{
		return (c < 0x80 || (c > 0xaf && (c < 0xe0 || c > 0xf1))) ? false : true;
	}
	public static boolean isletter1251(byte c)
	{
		return ((c >= 0xc0 && c <= 0xdf) || (c >= 0xe0 && c <= 0xff) || (c == 0xa8 || c == 0xb8));
	}
	public static void LOG_d(String msg)
	{
		Log.d("StyloQ", msg);
	}
	public static void LOG_i(String msg)
	{
		Log.i("StyloQ", msg);
	}
	public static void LOG_e(String msg)
	{
		Log.e("StyloQ", msg);
	}
	public static int satoi(String text)
	{
		int    result = 0;
		final  int len = GetLen(text);
		if(len > 0) {
			int    src_pos = 0;
			byte [] _p = text.getBytes();
			while(_p[src_pos] == ' ' || _p[src_pos] == '\t')
				src_pos++;
			boolean is_neg = false;
			boolean is_hex = false;
			if(_p[src_pos] == '-') {
				src_pos++;
				is_neg = true;
			}
			else if(_p[src_pos] == '+')
				src_pos++;
			if(src_pos < len) {
				if(_p[src_pos] == '0' && src_pos < (len - 1) && (_p[src_pos + 1] == 'x' || _p[src_pos + 1] == 'X')) {
					src_pos += 2;
					is_hex = true;
				}
				if(src_pos < len) {
					if(is_hex) {
						if(ishex(_p[src_pos])) {
							int local_len = 0;
							do {
								local_len++;
								if((src_pos + local_len) >= len)
									break;
							} while(ishex(_p[src_pos + local_len]));
							{
								result = 0;
								for(int i = 0; i < local_len; i++) {
									result = (result * 16) + hex(_p[src_pos + i]);
								}
							}
						}
					}
					else {
						if(isdec(_p[src_pos])) {
							int local_len = 0;
							do {
								local_len++;
								if((src_pos + local_len) >= len)
									break;
							} while(isdec(_p[src_pos + local_len]));
							{
								for(int i = 0; i < local_len; i++)
									result = (result * 10) + (_p[src_pos + i] - '0');
							}
						}
					}
					if(is_neg && result != 0)
						result = -result;
				}
			}
		}
		return result;
	}
	public static class SIntToSymbTabEntry {
		SIntToSymbTabEntry(int id, String symb)
		{
			Id = id;
			Symb = symb;
		}
		int    Id;
		String Symb;
	};
	public static final String SIntToSymbTab_GetSymb(final SIntToSymbTabEntry [] tab, int id)
	{
		if(tab != null) {
			for(int i = 0; i < tab.length; i++) {
				if(tab[i].Id == id)
					return tab[i].Symb;
			}
		}
		return null;
	}
	public static boolean SIntToSymbTab_HasId(final SIntToSymbTabEntry [] tab, int id)
	{
		boolean yes = false;
		if(tab != null) {
			for(int i = 0; !yes && i < tab.length; i++) {
				if(tab[i].Id == id)
					yes = true;
			}
		}
		return yes;
	}
	public static int SIntToSymbTab_GetId(final SIntToSymbTabEntry [] tab, final String symb)
	{
		if(tab != null && GetLen(symb) > 0) {
			for(int i = 0; i < tab.length; i++) {
				if(tab[i].Symb.equalsIgnoreCase(symb))
					return tab[i].Id;
			}
		}
		return 0;
	}
	public static class PPObjID {
		PPObjID()
		{
			Type = 0;
			Id = 0;
		}
		PPObjID(int type, int id)
		{
			Type = type;
			Id = id;
		}
		boolean IsEmpty() { return (Type == 0); }
		//
		// Descr: Идентифицирует тип и идентификатор объекта по строковым представлениям
		//   objtype and objid.
		// Note: Сейчас функция может идентифицировать не все типы объектов Papyrus,
		//   но лишь те, которые важны в рамках проекта Stylo-Q.
		//
		public static PPObjID Identify(String objtype, String objid)
		{
			PPObjID result = null;
			if(GetLen(objtype) > 0) {
				int type = satoi(objtype);
				int id = satoi(objid);
				if(type <= 0) {
					if(objtype.equalsIgnoreCase("unit"))
						type = PPOBJ_UNIT;
					else if(objtype.equalsIgnoreCase("quotkind"))
						type = PPOBJ_QUOTKIND;
					else if(objtype.equalsIgnoreCase("location"))
						type = PPOBJ_LOCATION;
					else if(objtype.equalsIgnoreCase("goods"))
						type = PPOBJ_GOODS;
					else if(objtype.equalsIgnoreCase("processor"))
						type = PPOBJ_PROCESSOR;
					else if(objtype.equalsIgnoreCase("bill"))
						type = PPOBJ_BILL;
					else if(objtype.equalsIgnoreCase("person"))
						type = PPOBJ_PERSON;
					else if(objtype.equalsIgnoreCase("styloqbindery"))
						type = PPOBJ_STYLOQBINDERY;
					else if(objtype.equalsIgnoreCase("goodsgroup"))
						type = PPOBJ_GOODSGROUP;
					else if(objtype.equalsIgnoreCase("brand"))
						type = PPOBJ_BRAND;
					else if(objtype.equalsIgnoreCase("currency"))
						type = PPOBJ_CURRENCY;
				}
				if(type > 0)
					result = new PPObjID(type, id);
			}
			return result;
		}
		int    Type;
		int    Id;
	}

	public static class ListViewEvent {
		int    ItemIdx;
		long   ItemId;   // for RecyclerView 0
		Object ItemObj;  // for RecyclerView null
		View   ItemView;
		//ViewGroup ParentView;
		RecyclerView.ViewHolder RvHolder; // for RecyclerView only
	}
	public static final int EV_NONE                 = 0;
	public static final int EV_CREATE               = 1; // (App, SlActivity) При создании объекта
	public static final int EV_TERMINTATE           = 2; // (App) При завершении работы объекта
	public static final int EV_COMMAND              = 3; // (SlActivity) при поступлении команды от управляющего элемента (button)
	public static final int EV_IADATAEDITCOMMIT     = 4; // После подтверждения пользователем факта интерактивного редактирования данных
	public static final int EV_GETLISTITEMVIEW      = 5; // arg(subj ListViewEvent) ret(resultView)
		// Если ((ListViewEvent)subj).RvHolder != 0, то функция HandleEvent должна заполнить этот холдер (речь идет о ReciclerView) и вернуть null.
	public static final int EV_LISTVIEWITEMCLK      = 6; // arg(subj ListViewEvent) ret(resultView)
	public static final int EV_LISTVIEWITEMLONGCLK  = 7; // arg(subj ListViewEvent) ret(resultView)
	public static final int EV_LISTVIEWCOUNT        = 8; // arg(subj ListViewEvent) ret(Long)
	public static final int EV_SERVICEREGISTERED    = 9; // arg(subj Long) ret(null)
	// Событие инициируется объектом SLib.RecyclerListAdapter при создании или для создания холдера.
	// Параметр subs является ссылкой на объект ListViewEvent.
	// -- Если холдер к моменту вызова события создан, то поле ListViewEvent.RvHolder не нулевое,
	// ListViewEvent.view указывает на элемент списка и получатель сообщения может провести какие-либо
	// манипуляции с холдером и элементом списка. Возвращаемый результат в этом случае игнорируется.
	// -- Если холдер к моменту вызова события НЕ создан, то поле ListViewEvent.RvHolder нулевое,
	// ListViewEvent.view указывает на родительский элемент холдера (ViewGroup) и получатель обязан создать
	// новый холдер и вернуть его в виде результата обработки события.
	public static final int EV_CREATEVIEWHOLDER     = 10; // arg(subj Long) ret(null)
	public static final int EV_CREATEFRAGMENT       = 11; // arg(subj Long) ret(resultFragment)
		// Вызывается для создания RecyclerViewHolder если не определен идентификатор ресурса элемента списка
	public static final int EV_SETVIEWDATA          = 12; // Посылается в activity для установки данных объекта view
	public static final int EV_GETVIEWDATA          = 13; // Посылается в activity для извлечения данных из объекта view
	public static final int EV_ACTIVITYRESULT       = 14; // Посылается после завершения работы SlActivity, запущенной
		// методом SlActivity.Launch(Intent). arg(subj ActivityResult), ret(null)
	public static final int EV_SETUPFRAGMENT        = 15; // Посылается функцией SlFragmentStatic::onCreate
		// родительскому Activity для предварительной установки
	public static final int EV_IADATADELETECOMMIT   = 16; // После подтверждения пользователем факта интерактивного удаления данных
	public static final int EV_DESTROY              = 17; // Посылается функцией onDestroy
	public static final int EV_ACTIVITYSTART        = 18; // Посылается в SlActivity функцией onStart
	public static final int EV_ACTIVITYRESUME       = 19; // Посылается в SlActivity функцией onResume
	public static final int EV_SVCQUERYRESULT       = 20; // @v11.3.10 Посылается в SlActivity после выполнения запроса к сервису по заданию этой activity
	public static final int EV_CBSELECTED           = 21; // @v11.4.0 Посылается в ответ на выбор элемента в combo-box'е (spinner)
	public static final int EV_TABSELECTED          = 22; // @v11.5.0 Посылается в ответ на выбор табулятора (ViewPager2) srcObj: ViewPager2; subj: Integer(tabIndex)
	//
	public static final int cmOK                    = 10; // Значение эквивалентно тому же в tvdefs.h
	public static final int cmCancel                = 11; // Значение эквивалентно тому же в tvdefs.h
	//
	// Descr: Идентификаторы видов операций EDI (соответствуют таким же символам в ppdefs.h)
	//
	public static final int PPEDIOP_NOP          = 0; // Не EDI-операция //
	public static final int PPEDIOP_ORDER        = 1; // Заказ поставщику
	public static final int PPEDIOP_ORDERRSP     = 2; // Подтверждение заказа от поставщика
	public static final int PPEDIOP_APERAK       = 3; // Статус обработки заказа поставщиком
	public static final int PPEDIOP_DESADV       = 4; // Уведомление от отгрузки поставщиком
	public static final int PPEDIOP_RECADV       = 5; // Подтверждение получения товара от поставщика
	public static final int PPEDIOP_DECLINEORDER = 6; // Отклонение заказа ранее высланного поставщику после получения подтверждения //
	public static final int PPEDIOP_ALCODESADV   = 7; // Дополнительный DESADV-документ с атрибутами, необходимыми для декларирования алкогольной продукции
	public static final int PPEDIOP_SALESORDER   = 8; // Заказ от покупателя
	public static final int PPEDIOP_PARTIN       = 9; //
	public static final int PPEDIOP_INVOIC       = 10; //
	public static final int PPEDIOP_RETURNREQ    = 11; //

	public static final long Ssc_CompressionSignature = 0xFDB975312468ACE0L;
	//
	// Descr: Идентификаторы типов документов обмена (соответствуют символам в pp.h)
	//
	public static final int sqbdtNone       = 0;
	// Диапазон [1..500] зарезервирован за типами документов EDI в соответствии с идентификацией PPEDIOP_XXX
	// Только небольшая часть из тех операций будут применены для Stylo-Q, но для обеспечения соответствия и
	// минимизации дублирования мы резервируем диапазон с запасом.
	public static final int sqbdtCCheck     = 501; // Кассовый чек
	public static final int sqbdtContact    = 502; // Контактные данные персоналии
	public static final int sqbdtTodo       = 503; // Задача (todo)
	public static final int sqbdtSvcReq     = 504; // Запрос на обслуживание

	//
	// Descr: Агрегатный функции для комбинирования выборки значений
	// Копия значений, определенных в проекте Papyrus (slib.h)
	// @persistent
	//
	public static final int AGGRFUNC_NONE       = 0;
	public static final int AGGRFUNC_COUNT      = 1;
	public static final int AGGRFUNC_SUM        = 2;
	public static final int AGGRFUNC_AVG        = 3; // Среднее арифметическое
	public static final int AGGRFUNC_MIN        = 4;
	public static final int AGGRFUNC_MAX        = 5;
	public static final int AGGRFUNC_STDDEV     = 6; // Стандартное отклонение
	//
	// Common format flags
	//
	public static final int ALIGN_RIGHT    = 0x1000;  // Выравнивать строку вправо
	public static final int ALIGN_LEFT     = 0x2000;  // Выравнивать строку влево
	public static final int ALIGN_CENTER   = 0x3000;  // Центрировать строку
	public static final int COMF_FILLOVF   = 0x4000;  // Если результирующая строка не помещается в указанную длину, то заполнять символами '#'
	public static final int COMF_SQL       = 0x8000;  // Форматировать значения в пригодном для SQL виде
		// Этот флаг передается в функции форматирования различных типов для того, чтобы они смогли
		// правильно сформировать строковое представление данных.
	//
	// Structure of format field:
	//   Precision           4 bit
	//   Text output length 12 bit
	//   Flags              16 bit
	//
	public static final int SFFLAGMASK   = 0x0000ffff;
	public static final int SFLENMASK    = 0x0fff0000;
	public static final int SFPRCMASK    = 0xf0000000;
	public static final int SFALIGNMASK  = (ALIGN_LEFT|ALIGN_RIGHT|ALIGN_CENTER);
	public static final int SFCOMMONMASK = (SFALIGNMASK|COMF_FILLOVF);

	public static int MKSFMT(int l, int f) { return (l<<16) | (f & 0x0000ffff); }
	public static int MKSFMTD(int l, int p, int f) { return ((p<<28)|(l<<16)|(f & 0x0000ffff)); }
	public static int SFMTFLAG(int fmt) { return (fmt & SFFLAGMASK); }
	public static int SFMTLEN(int fmt) { return ((fmt & SFLENMASK)>>16); }
	public static int SFMTPRC(int fmt) { return ((fmt&SFPRCMASK)>>28); }
	public static int SFMTALIGN(int fmt) { return (fmt&SFALIGNMASK); }
	//
	// Date format flags
	//
	public static final int DATF_AMERICAN =      1;  // mm/dd/yy
	public static final int DATF_ANSI     =      2;  // yy.mm.dd
	public static final int DATF_BRITISH  =      3;  // dd/mm/yy
	public static final int DATF_FRENCH   =      4;  // dd/mm/yy
	public static final int DATF_GERMAN   =      5;  // dd.mm.yy
	public static final int DATF_ITALIAN  =      6;  // dd-mm-yy
	public static final int DATF_JAPAN    =      7;  // yy/mm/dd
	public static final int DATF_USA      =      8;  // mm-dd-yy
	public static final int DATF_MDY      =      9;  // mm/dd/yy
	public static final int DATF_DMY      =     10;  // dd/mm/yy
	public static final int DATF_YMD      =     11;  // yy/mm/dd
	public static final int DATF_SQL      =     12;  // DATE 'YYYY-MM-DD'
	public static final int DATF_INTERNET =     13;  // Wed, 27 Feb 2008
	public static final int DATF_ISO8601  =     14;  // yyyy-mm-dd
	public static final int DATF_CENTURY  = 0x0010;  // Century flag
	public static final int DATF_NOZERO   = 0x0020;  // Не отображать нулевое значение
	public static final int DATF_NODIV    = 0x0040;  // день, месяц и год следуют друг за другом без разделителей.
			// Порядок следования определяется младшими битами (DATF_AMERICAN..DATF_YMD). Если установлен
			// флаг DATF_CENTURY, то год должен быть полным (2011), в противном случае - только последние две
			// цифры года (11).
	//
	// Time format flags
	//
	public static final int TIMF_HMS      =      1;  // 23:20:59
	public static final int TIMF_HM       =      2;  // 23:20
	public static final int TIMF_MS       =      3;  // 20:59
	public static final int TIMF_S        =      4;  // 59
	public static final int TIMF_SQL      =      5;  // TIMESTAMP 'YYYY-MM-DD HH:MM:SS.HS'
	public static final int TIMF_MSEC     = 0x0008;  // 23:20:59.019
	public static final int TIMF_BLANK    = 0x0010;  //
	public static final int TIMF_NOZERO   = TIMF_BLANK; // Не отображать нулевое значение
	public static final int TIMF_TIMEZONE = 0x0020;  // +0300
	public static final int TIMF_NODIV    = 0x0040;  //
	public static final int TIMF_DOTDIV   = 0x0080;  // Разделители часов, минут и секунд - точки. Например: 23.20.59
	//
	static public int LoByte(int w) { return (w & 0x000000ff); }
	static public int HiByte(int w) { return ((w >> 8) & 0x000000ff); }
	static public int LoWord(int L) { return (L & 0x0000ffff); }
	static public int HiWord(int L) { return ((L >> 16) & 0x0000ffff); }
	static public int MakeInt(int low, int high) { return ((low & 0x0000ffff) | ((high << 16) & 0xffff0000)); }
	//
	//
	//
	//
	public static void THROW(boolean condition, int errCode) throws StyloQException
	{
		if(!condition)
			throw new StyloQException(errCode, "");
	}
	public static void THROW(boolean condition, int errCode, String addedMsg) throws StyloQException
	{
		if(!condition)
			throw new StyloQException(errCode, addedMsg);
	}
	public static void THROW_SL(boolean condition, int errCode) throws StyloQException
	{
		if(!condition)
			throw new StyloQException(ppstr2.PPSTR_SLIBERR, errCode, "");
	}
	public static int BytesToInt(byte [] buf, int offs)
	{
		return (buf[offs] & 0xff) | ((buf[offs+1] & 0xff) << 8) | ((buf[offs+2] & 0xff) << 16) | ((buf[offs+3] & 0xff) << 24);
	}
	public static long BytesToLong(byte [] buf, int offs)
	{
		return
				(buf[offs] & 0xffL) |
						((buf[offs+1] & 0xffL) << 8) |
						((buf[offs+2] & 0xffL) << 16) |
						((buf[offs+3] & 0xffL) << 24) |
						((buf[offs+4] & 0xffL) << 32) |
						((buf[offs+5] & 0xffL) << 40) |
						((buf[offs+6] & 0xffL) <<  48) |
						((buf[offs+7] & 0xffL) <<  56);
	}
	public static byte [] LongToBytes(long value)
	{
		byte [] buf = new byte[Long.BYTES];
		buf[0] = (byte)(value & 0xffL);
		buf[1] = (byte)((value >> 8) & 0xffL);
		buf[2] = (byte)((value >> 16) & 0xffL);
		buf[3] = (byte)((value >> 24) & 0xffL);
		buf[4] = (byte)((value >> 32) & 0xffL);
		buf[5] = (byte)((value >> 40) & 0xffL);
		buf[6] = (byte)((value >> 48) & 0xffL);
		buf[7] = (byte)((value >> 56) & 0xffL);
		return buf;
	}
	public static short BytesToShort(byte [] buf, int offs)
	{
		return (short)((buf[offs] & 0xff) | ((buf[offs+1] & 0xff) << 8));
	}
	public static String Copy(final String s)
	{
		return (s != null) ? ("" + s) : null;
	}
	public static byte [] Copy(final byte [] s)
	{
		return (s != null) ? s.clone() : null;
	}
	public static UUID Copy(final UUID s)
	{
		return (s != null) ? new UUID(s.getMostSignificantBits(), s.getLeastSignificantBits()) : null;
	}
	public static String ReadSString(InputStream stream) throws StyloQException
	{
		String result = null;
		if(stream != null) {
			try {
				int avl = stream.available();
				if(avl >= 2) {
					byte[] temp_buf = new byte[2]; // Длина строки записана в поток как 2-х байтовое целое!
					if(stream.read(temp_buf) == temp_buf.length) {
						short size = SLib.BytesToShort(temp_buf, 0);
						if(size == 0) {
							result = "";
						}
						else if(size > 0) {
							temp_buf = new byte[size];
							if(stream.read(temp_buf) == size)
								result = new String(temp_buf);
						}
					}
				}
			} catch(IOException exn) {
				result = null;
				throw new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
			}
		}
		return result;
	}
	public static byte [] ReadSBuffer(InputStream stream) throws StyloQException
	{
		byte [] result = null;
		if(stream != null) {
			try {
				int avl = stream.available();
				if(avl >= 4) {
					byte[] temp_buf = new byte[4];
					if(stream.read(temp_buf) == temp_buf.length) {
						int size = SLib.BytesToInt(temp_buf, 0);
						if(size == 0)
							result = new byte[0];
						else if(size > 0) {
							result = new byte[size];
							if(stream.read(result) != size)
								result = null;
						}
					}
				}
			} catch(IOException exn) {
				result = null;
				throw new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
			}
		}
		return result;
	}
	public static SLib.StringSet ReadStringSet(InputStream stream) throws StyloQException
	{
		SLib.StringSet result = new SLib.StringSet();
		return result.Read(stream) ? result : null;
	}
	public static int [] ReadIntArray(InputStream stream) throws StyloQException
	{
		int [] result = null;
		if(stream != null) {
			try {
				int avl = stream.available();
				if(avl >= 8) {
					byte[] temp_buf = new byte[8];
					if(stream.read(temp_buf) == temp_buf.length) {
						int item_size = SLib.BytesToInt(temp_buf, 0);
						int count = SLib.BytesToInt(temp_buf, 4);
						if(item_size == 4) {
							if(count == 0) {
								result = new int[0];
							}
							else if(count > 0) {
								result = new int[count];
								for(int i = 0; i < count; i++) {
									if(stream.read(temp_buf) == item_size) {
										result[i] = BytesToInt(temp_buf, 0);
									}
									else {
										result = null;
										break;
									}
								}
							}
						}
					}
				}
			} catch(IOException exn) {
				result = null;
				throw new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
			}
		}
		return result;
	}
	public static class LAssocVector {
		public LAssocVector()
		{
			D = null;
			Count = 0;
		}
		public int getCount() { return Count; }
		public SLib.LAssocVector Z()
		{
			Count = 0;
			return this;
		}
		private int SnapUpSize(int i)
		{
			if(i <= 12)
				i = 12;
			else {
				int j = i;
				// Assumes your system is at least 32 bits, and your string is
				// at most 4GB is size
				j |= (j >>  1);
				j |= (j >>  2);
				j |= (j >>  4);
				j |= (j >>  8);
				j |= (j >> 16);
				i = j + 1; // Least power of two greater than i
			}
			return i;
		}
		private boolean Alloc(int newSize)
		{
			boolean ok = true;
			if(newSize <= 0)
				Z();
			else if(newSize > GetLen(D)) {
				int new_size = SnapUpSize(newSize);
				SLib.LAssoc[] new_buf = new SLib.LAssoc[new_size];
				if(GetLen(D) > 0)
					System.arraycopy(D, 0, new_buf, 0, Count);
				D = new_buf;
			}
			return ok;
		}
		SLib.LAssoc at(int idx)
		{
			return (D != null && idx >= 0 && idx < Count) ? D[idx] : null;
		}
		//
		// Descr: Вставляет в конец вектора новый элемент newItem
		// Returns: Индекс нового элемента в векторе.
		//
		int insert(SLib.LAssoc newItem)
		{
			int result_idx = -1;
			int new_count = Count + 1;
			if(new_count > GetLen(D)) {
				Alloc(new_count);
			}
			result_idx = Count;
			D[result_idx] = newItem;
			Count++;
			return result_idx;
		}
		public void Add(int key, int val)
		{
			insert(new LAssoc(key, val));
		}
		public int AddUnique(int key, int val)
		{
			if(Search(key) >= 0)
				return -1;
			else {
				Add(key, val);
				return 1;
			}
		}
		public int  Remove(int key)
		{
			int idx = Search(key);
			if(idx >= 0) {
				assert(idx < Count);
				atFree(idx);
				return 1;
			}
			else
				return -1;
		}
		public void atFree(int idx)
		{
			if(Count > 0 && idx < Count && idx >= 0) {
				if(idx == Count-1) {
					Count--;
				}
				else {
					System.arraycopy(D, idx+1, D, idx, Count-idx+1);
					Count--;
				}
			}
		}
		public boolean Read(InputStream stream) throws StyloQException
		{
			boolean result = true;
			Z();
			if(stream != null) {
				try {
					int avl = stream.available();
					if(avl >= 8) {
						byte[] temp_buf = new byte[8];
						if(stream.read(temp_buf) == temp_buf.length) {
							int item_size = BytesToInt(temp_buf, 0);
							int count = BytesToInt(temp_buf, 4);
							if(item_size == 8) {
								if(count == 0) {
									D = new SLib.LAssoc[0];
								}
								else if(count > 0) {
									D = new SLib.LAssoc[count];
									for(int i = 0; i < count; i++) {
										if(stream.read(temp_buf) == temp_buf.length) {
											D[i] = new SLib.LAssoc();
											D[i].Key = BytesToInt(temp_buf, 0);
											D[i].Value = BytesToInt(temp_buf, 4);
											Count++;
										}
										else {
											Z();
											result = false;
											break;
										}
									}
								}
							}
						}
					}
				} catch(IOException exn) {
					Z();
					result = false;
					throw new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
				}
			}
			return result;
		}
		//
		// Descr: Ищет ключ key в массиве ассоциаций array и, если находит, то
		//   возвращает позицию ключа [0..array.length-1]. Если не находит, то
		//   возвращает -1.
		//
		public int Search(int key)
		{
			if(D != null && Count > 0) {
				for(int i = 0; i < Count; i++)
					if(D[i].Key == key)
						return i;
			}
			return -1;
		}
		public int SearchValue(int value)
		{
			if(D != null && Count > 0) {
				for(int i = 0; i < Count; i++)
					if(D[i].Value == value)
						return i;
			}
			return -1;
		}
		private static class _Comparator implements Comparator<LAssoc> {
			public int compare(LAssoc a, LAssoc b)
			{
				if(a.Key < b.Key)
					return -1;
				else if(a.Key > b.Key)
					return +1;
				else if(a.Value < b.Value)
					return -1;
				else if(a.Value > b.Value)
					return +1;
				else
					return 0;
			}
		}
		public void Sort()
		{
			if(Count > 1)
				Arrays.sort(D, 0, Count-1, new _Comparator());
		}
		SLib.LAssoc D[];
		int   Count;
	}
	static boolean IsBytesZero(byte [] bytes)
	{
		for(int i = 0; i < bytes.length; i++)
			if(bytes[i] != 0)
				return false;
		return true;
	}
	static boolean AreBytesSame(byte [] text, int textOffs, byte [] pattern)
	{
		boolean result = false;
		if(text != null) {
			int pl = GetLen(pattern);
			if(pl > 0) {
				int tl = text.length;
				if(tl > textOffs) {
					tl = tl - textOffs;
					if(pl <= tl) {
						result = true;
						for(int i = 0; i < pl; i++) {
							if(text[textOffs+i] != pattern[i]) {
								result = false;
								break;
							}
						}
					}
				}
			}
		}
		return result;
	}
	static boolean AreBytesEqualsToString(byte [] text, int textOffs, int textLen, String pattern, boolean ignoreCase)
	{
		if(text != null && pattern != null && textOffs >= 0 && textLen > 0 && (textOffs+textLen) <= text.length) {
			byte[] temp_buf = new byte[textLen];
			for(int i = 0; i < textLen; i++) {
				temp_buf[i] = text[textOffs+i];
			}
			String temp_str = new String(temp_buf);
			return ignoreCase ? pattern.equalsIgnoreCase(temp_str) : pattern.equals(temp_str);
		}
		else
			return false;
	}
	static public class StringSet {
		StringSet()
		{
			Buf = null;
			Delim = null;
			DataLen = 0;
		}
		@org.checkerframework.checker.nullness.qual.NonNull StringSet Z()
		{
			Buf = null;
			DataLen = 0;
			return this;
		}
		private int SnapUpSize(int i)
		{
			if(i <= 12)
				i = 12;
			else {
				int j = i;
				// Assumes your system is at least 32 bits, and your string is
				// at most 4GB is size
				j |= (j >>  1);
				j |= (j >>  2);
				j |= (j >>  4);
				j |= (j >>  8);
				j |= (j >> 16);
				i = j + 1; // Least power of two greater than i
			}
			return i;
		}
		private boolean Alloc(int newSize)
		{
			boolean ok = true;
			if(newSize <= 0)
				Z();
			else if(newSize > GetLen(Buf)) {
				int new_size = SnapUpSize(newSize);
				byte [] new_buf = new byte[new_size];
				if(GetLen(Buf) > 0) {
					System.arraycopy(Buf, 0, new_buf, 0, Buf.length);
				}
				Buf = new_buf;
			}
			return ok;
		}
		//
		// Descr: Добавляет в конец набора новую строку newStr.
		// Returns:
		//  >= 0 - начальная позиция добавленной строки
		//  <0 - ошибка
		//
		public int Add(String newStr)
		{
			int result_pos = -1;
			byte [] new_bytes = (GetLen(newStr) == 0) ? new byte[0] : newStr.getBytes();
			final int delim_len = (DataLen > 0) ? (GetLen(Delim) > 0 ? Delim.length() : 1) : (GetLen(Delim) > 0 ? 1 : 2);
			final int add_len   = new_bytes.length;
			final int new_len   = DataLen + add_len + delim_len;
			int   p;
			if((Buf != null && new_len <= Buf.length) || Alloc(new_len)) {
				if(DataLen == 0) {
					p = 0;
					System.arraycopy(new_bytes, 0, Buf, p, add_len);
					if(GetLen(Delim) == 0)
						Buf[add_len+1] = 0;
				}
				else {
					p = DataLen - 1;
					if(GetLen(Delim) > 0) {
						System.arraycopy(Delim, 0, Buf, p, GetLen(Delim));
						p += delim_len;
						System.arraycopy(new_bytes, 0, Buf, p, add_len);
					}
					else {
						System.arraycopy(new_bytes, 0, Buf, p, add_len);
						Buf[new_len-1] = 0;
					}
				}
				result_pos = p;
				DataLen = new_len;
			}
			else
				result_pos = -1;
			return result_pos;
		}
		public boolean Read(InputStream stream) throws StyloQException
		{
			boolean ok = true;
			if(stream != null) {
				try {
					int avl = stream.available();
					if(avl >= 8) {
						byte[] temp_buf = new byte[4];
						if(stream.read(temp_buf) == temp_buf.length) {
							int total_size = BytesToInt(temp_buf, 0);
							Delim = ReadSString(stream);
							if(Delim != null) {
								Buf = ReadSBuffer(stream);
								if(Buf != null) {
									DataLen = Buf.length;
									; // ok
								}
								else
									ok = false;
							}
							else
								ok = false;
						}
					}
				} catch(IOException exn) {
					ok = false;
					throw new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
				}
			}
			return ok;
		}
		@org.checkerframework.checker.nullness.qual.NonNull String GetNZ(int pos) { return (pos > 0) ? Get(pos) : ""; }
		int Search(int startIdx, String pattern, boolean ignoreCase)
		{
			int idx = -1;
			assert(Buf == null || DataLen <= Buf.length);
			assert(Buf != null || DataLen == 0);
			if(Buf != null) {
				int pattern_len = GetLen(pattern);
				int buf_len = DataLen;
				if(pattern_len > 0 && startIdx < buf_len) {
					int p = startIdx;
					int delim_len = GetLen(Delim);
					if(delim_len > 0) {
						byte[] delim_bytes = Delim.getBytes();
						int result_len = 0;
						int start_pos = p;
						for(int i = p; i < buf_len; i++) {
							if(!AreBytesSame(Buf, i, delim_bytes))
								result_len++;
							else {
								if(AreBytesEqualsToString(Buf, start_pos, result_len, pattern, ignoreCase)) {
									return start_pos;
								}
								else {
									start_pos = i + delim_len;
									result_len = 0;
								}
							}
						}
					}
					else {
						delim_len = 1;
						int result_len = 0;
						int start_pos = p;
						for(int i = p; i < buf_len; i++) {
							if(Buf[i] != 0)
								result_len++;
							else {
								if(result_len > 0) {
									if(AreBytesEqualsToString(Buf, start_pos, result_len, pattern, ignoreCase))
										return start_pos;
									else {
										start_pos = i + delim_len;
										result_len = 0;
									}
								}
								else
									break; // Мы встретили двойной '0' - это окончание сета
							}
						}
					}
				}
			}
			return -1;
		}
		String Get(int pos)
		{
			String result = null;
			int p = pos;
			assert(Buf == null || DataLen <= Buf.length);
			assert(Buf != null || DataLen == 0);
			if(Buf != null && p < DataLen) {
				int delim_len = GetLen(Delim);
				int result_len = 0;
				if(delim_len > 0) {
					byte [] delim_bytes = Delim.getBytes();
					for(int i = p; i < DataLen; i++) {
						if(!AreBytesSame(Buf, i, delim_bytes))
							result_len++;
						else
							break;
					}
				}
				else {
					delim_len = 1;
					for(int i = p; i < DataLen; i++) {
						if(Buf[i] != 0)
							result_len++;
						else
							break;
					}
				}
				if(result_len > 0) {
					byte[] temp_buf = new byte[result_len];
					for(int i = 0; i < result_len; i++) {
						temp_buf[i] = Buf[p+i];
					}
					result = new String(temp_buf);
				}
				else
					result = "";
			}
			return result;
		}
		private byte [] Buf;
		private int DataLen;
		private String Delim;
	}
	static public class StrAssocArray {
		public static class Item {
			int Id;
			int ParentId;
			String Txt;
		}
		StrAssocArray()
		{
			Ss = new StringSet();
			Ss.Add("$"); // zero index - is empty string
			List = new LAssocVector();
			ParentList = new LAssocVector();
		}
		StrAssocArray Z()
		{
			List.Z();
			ParentList.Z();
			Ss.Z();
			Ss.Add("$"); // zero index - is empty string
			return this;
		}
		int GetCount() { return List.getCount(); }
		//
		// Descr: Добавляет либо меняет значение строки txt, ассоциированное
		//   с идентификатором id. Если такой идентификатор уже есть и соответствующая
		//   ему строка не равна txt, то заменяет существующую строку на txt.
		//   Если идентификатор отсутствует, то вставляет новый элемент {id, 0/*parent*/, txt}.
		// Returns: Индекс найденного либо вновь добавленного элемента.
		//
		int Set(int id, String txt)
		{
			int   new_idx = -1;
			int   ex_idx = Search(id);
			if(ex_idx >= 0) {
				String ex_txt = Ss.GetNZ(List.at(ex_idx).Value);
				if(!ex_txt.equals(txt)) {
					int new_ss_pos = Ss.Add(txt);
					List.at(ex_idx).Value = (new_ss_pos >= 0) ? new_ss_pos : 0;
				}
				new_idx = ex_idx;
			}
			else {
				int new_ss_pos = Ss.Add(txt);
				new_idx = List.insert(new LAssoc(id, (new_ss_pos >= 0) ? new_ss_pos : 0));
			}
			return new_idx;
		}
		StrAssocArray.Item GetByPos(int idx)
		{
			if(idx < List.getCount()) {
				StrAssocArray.Item item = new StrAssocArray.Item();
				item.Id = List.at(idx).Key;
				int pos2 = List.at(idx).Value;
				if(pos2 >= 0) {
					item.Txt = Ss.Get(pos2);
				}
				return item;
			}
			else
				return null;
		}
		int GetTextPos(int pos)
		{
			int   tpos = 0;
			if(pos >= 0 && pos < List.getCount()) {
				//LAssoc & r_assc = List[pos];
				tpos = List.at(pos).Value;
				if(tpos <= 0 || tpos >= Ss.Buf.length)
					tpos = 0;
			}
			return tpos;
		}
		String GetTextByPos(int textPos)
		{
			return (textPos > 0 && textPos < Ss.Buf.length) ? Ss.Get(textPos) : null;
		}
		String GetText(int ident)
		{
			String result = null;
			//int idx = SearchLAssocArray(List, ident);
			int idx = List.Search(ident);
			if(idx >= 0)
				result = Ss.GetNZ(List.at(idx).Value);
			return result;
		}
		int Search(int id)
		{
			return List.Search(id);
		}
		int SearchByText(String text)
		{
			int start_pos = 0;
			int idx = Ss.Search(start_pos, text, false);
			if(idx > 0) {
				//int lp = SearchLAssocArrayValue(List, idx);
				int lp = List.SearchValue(idx);
				if(lp > 0)
					return lp;
			}
			return -1;
		}
		boolean Read(InputStream stream) throws StyloQException
		{
			boolean ok = false;
			Z();
			if(stream != null) {
				//List = ReadLAssocArray(stream);
				List = new LAssocVector();
				if(List.Read(stream)) {
					//ParentList = ReadLAssocArray(stream);
					ParentList = new LAssocVector();
					if(ParentList.Read(stream)) {
						Ss = ReadStringSet(stream);
						if(Ss != null)
							ok = true;
						else {
							ParentList = null;
							List = null;
						}
					}
					else
						List = null;
				}
			}
			return ok;
		}
		LAssocVector List;
		LAssocVector ParentList;
		StringSet Ss;
	}

	public static boolean IsEmpty(final String s) { return (s == null || s.length() == 0); }
	public static boolean IsInstanceOf(Object obj, Class cls) { return (obj != null && cls.isInstance(obj)); }
	public static int GetLen(final String s) { return (s != null) ? s.length() : 0; }
	public static int GetLen(final CharSequence s) { return (s != null) ? s.length() : 0; }
	public static int GetCount(final ArrayList<?> list) { return (list != null) ? list.size() : 0; }
	public static <T> int GetLen(final T [] s)
	{
		return (s != null) ? s.length : 0;
	}
	public static int GetLen(final byte [] s)
	{
		return (s != null) ? s.length : 0;
	}
	public static boolean IsInRange(int idx, ArrayList<?> list) { return (list != null && idx >= 0 && idx < list.size()); }
	public static boolean AreUUIDsEqual(final UUID a1, final UUID a2)
	{
		if(a1 == null)
			if(a2 == null)
				return true;
			else
				return false;
		else if(a2 == null)
			return false;
		else
			return a1.equals(a2);
	}
	public static boolean AreStringsEqual(final String a1, final String a2)
	{
		if(a1 == null)
			if(a2 == null)
				return true;
			else
				return false;
		else if(a2 == null)
			return false;
		else {
			final int len1 = a1.length();
			final int len2 = a2.length();
			if(len1 != len2)
				return false;
			else if(len1 == 0)
				return true;
			else
				return a1.equals(a2);
		}
	}
	public static boolean AreStringsEqualNoCase(final String a1, final String a2)
	{
		if(a1 == null)
			if(a2 == null)
				return true;
			else
				return false;
		else if(a2 == null)
			return false;
		else {
			final int len1 = a1.length();
			final int len2 = a2.length();
			if(len1 != len2)
				return false;
			else if(len1 == 0)
				return true;
			else
				return a1.equalsIgnoreCase(a2);
		}
	}
	public static boolean AreByteArraysEqual(final byte [] a1, final byte [] a2)
	{
		if(a1 == null)
			if(a2 == null)
				return true;
			else
				return false;
		else if(a2 == null)
			return false;
		else {
			final int len1 = a1.length;
			final int len2 = a2.length;
			if(len1 != len2)
				return false;
			else if(len1 == 0)
				return true;
			else {
				for(int i = 0; i < len1; i++) {
					if(a1[i] != a2[i])
						return false;
				}
				return true;
			}
		}
	}
	public static byte [] BigNumberToBytesWithoutLZ(BigInteger bn)
	{
		if(bn != null) {
			byte [] byte_list = bn.toByteArray();
			if(byte_list != null && byte_list.length > 1 && byte_list[0] == 0) {
				byte[] result = new byte[byte_list.length-1];
				System.arraycopy(byte_list, 1, result, 0, byte_list.length-1);
				return result;
			}
			else
				return byte_list;
		}
		else
			return null;
	}
	public static BigInteger GenerateRandomBigNumber(int numBits)
	{
		Random rand = new Random();
		BigInteger bn = null;
		//do {
		bn = new BigInteger(numBits, rand);
		//} while(bn.toByteArray().length > (numBits / 8));
		return bn;
	}
	public static String ByteToHex(byte num)
	{
		char[] hex_dig = new char[2];
		hex_dig[0] = Character.forDigit((num >> 4) & 0xF, 16);
		hex_dig[1] = Character.forDigit((num & 0xF), 16);
		return new String(hex_dig);
	}
	public static String ByteArrayToHexString(byte[] byteArray)
	{
		StringBuffer hexStringBuffer = new StringBuffer();
		for(int i = 0; i < byteArray.length; i++) {
			hexStringBuffer.append(ByteToHex(byteArray[i]));
		}
		return hexStringBuffer.toString();
	}
	public static int GetLinguaIdent(String langSymb)
	{
		// @todo Это - временная реализация. Необходимо сделать полную табличную версию.
		if(GetLen(langSymb) > 0) {
			if(langSymb.equalsIgnoreCase("en"))
				return slangEN;
			else if(langSymb.equalsIgnoreCase("ru"))
				return slangRU;
			else if(langSymb.equalsIgnoreCase("de"))
				return slangDE;
			else if(langSymb.equalsIgnoreCase("es"))
				return slangES;
			else if(langSymb.equalsIgnoreCase("fr"))
				return slangFR;
			else if(langSymb.equalsIgnoreCase("pt"))
				return slangPT;
			else if(langSymb.equalsIgnoreCase("nl"))
				return slangNL;
			else
				return 0;
		}
		else
			return 0;
	}
	public static String GetLinguaCode(int lang)
	{
		// @todo Это - временная реализация. Необходимо сделать полную табличную версию.
		if(lang == slangEN)
			return "en";
		else if(lang == slangRU)
			return "ru";
		else if(lang == slangDE)
			return "de";
		else if(lang == slangES)
			return "es";
		else if(lang == slangFR)
			return "fr";
		else if(lang == slangPT)
			return "pt";
		else if(lang == slangNL)
			return "nl";
		return null;
	}
	//
	//
	//
	public static int DateToDaysSinceChristmas(int y, int m, int d)
	{
		int    n = 0;
		//
		// Add days for months in given date
		//
		switch(m) {
			case  1:
				if((y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0))
					n--;
				break;
			case  2:
				if((y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0))
					n += 30;
				else
					n += 31;
				break;
			case  3: n += 31+28; break;
			case  4: n += 31+28+31; break;
			case  5: n += 31+28+31+30; break;
			case  6: n += 31+28+31+30+31; break;
			case  7: n += 31+28+31+30+31+30; break;
			case  8: n += 31+28+31+30+31+30+31; break;
			case  9: n += 31+28+31+30+31+30+31+31; break;
			case 10: n += 31+28+31+30+31+30+31+31+30; break;
			case 11: n += 31+28+31+30+31+30+31+31+30+31; break;
			case 12: n += 31+28+31+30+31+30+31+31+30+31+30; break;
		}
		// Since every leap year is of 366 days,
		// Add a day for every leap year
		return d + n + ((y-1) * 365) + ((y / 4) - (y / 100) + (y / 400));
	}
	//
	//  The following two tables map a month index to the number of days preceding
	//  the month in the year.  Both tables are zero based.  For example, 1 (Feb)
	//  has 31 days preceding it.  To help calculate the maximum number of days
	//  in a month each table has 13 entries, so the number of days in a month
	//  of index i is the table entry of i+1 minus the table entry of i.
	//
	private static final int LeapYearDaysPrecedingMonth[] = {
		0,                             // January
		31,                            // February
		31+29,                         // March
		31+29+31,                      // April
		31+29+31+30,                   // May
		31+29+31+30+31,                // June
		31+29+31+30+31+30,             // July
		31+29+31+30+31+30+31,          // August
		31+29+31+30+31+30+31+31,       // September
		31+29+31+30+31+30+31+31+30,    // October
		31+29+31+30+31+30+31+31+30+31, // November
		31+29+31+30+31+30+31+31+30+31+30, // December
		31+29+31+30+31+30+31+31+30+31+30+31
	};
	private static final int NormalYearDaysPrecedingMonth[] = {
		0,                             // January
		31,                            // February
		31+28,                         // March
		31+28+31,                      // April
		31+28+31+30,                   // May
		31+28+31+30+31,                // June
		31+28+31+30+31+30,             // July
		31+28+31+30+31+30+31,          // August
		31+28+31+30+31+30+31+31,       // September
		31+28+31+30+31+30+31+31+30,    // October
		31+28+31+30+31+30+31+31+30+31, // November
		31+28+31+30+31+30+31+31+30+31+30, // December
		31+28+31+30+31+30+31+31+30+31+30+31
	};
	//
	//  The following two tables map a day offset within a year to the month
	//  containing the day.  Both tables are zero based.  For example, day
	//  offset of 0 to 30 map to 0 (which is Jan).
	//
	private static final int LeapYearDayToMonth[] = {
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // January
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // February
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // March
			3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  // April
			4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, // May
			5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,  // June
			6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, // July
			7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, // August
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,  // September
			9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, // October
			10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, // November
			11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11
	};                                                                                                 // December
	private static final int NormalYearDayToMonth[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // January
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,        // February
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // March
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  // April
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, // May
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,  // June
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, // July
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, // August
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,  // September
		9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, // October
		10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, // November
		11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11
	};                                                                                                 // December
	private static LDATE DaysSinceChristmasToDate(int g)
	{
		int    day = 0;
		int    _year = 0;
		int    mon = 0;
		if(g >= 0) {
			final int    h4y = g / (400 * 365 + 97);
			final int    h4d_rem = g % (400 * 365 + 97);
			final int    hy = h4d_rem / (100 * 365 + 24);
			final int    hy_rem = h4d_rem % (100 * 365 + 24);
			final int    fy = hy_rem / (4 * 365 + 1);
			final int    fy_rem = hy_rem % (4 * 365 + 1);
			final int    y = fy_rem / 365;
			day = fy_rem % 365;
			_year = (h4y * 400) + (hy * 100) + (fy * 4) + y + 1;
			mon = 0;
			if(day == 0) {
				_year--;
				mon = 12;
				day = (y == 4 || hy == 4) ? 30 : 31; // Граничная проблема: конец четверки годов или 400-летия
			}
			else if(IsLeapYear(_year)) {
				mon = LeapYearDayToMonth[day-1];
				day -= LeapYearDaysPrecedingMonth[mon];
				mon++;
			}
			else {
				mon = NormalYearDayToMonth[day-1];
				day -= NormalYearDaysPrecedingMonth[mon];
				mon++;
			}
		}
		return new LDATE(day, mon, _year);
	}
	public static class LDATE {
		private static final int REL_DATE_MASK      = 0x80000000;
		private static final int THRSMDAY_DATE_MASK = 0x20000000;
		private static final int ANY_DATE_VALUE     = 0x7d0a7d7d;
		private static final int ANY_DAYITEM_VALUE  = 0x7d;
		private static final int ANY_MONITEM_VALUE  = 0x7d;
		private static final int ANY_YEARITEM_VALUE = 0x7d0a;

		public static boolean ArEq(final LDATE a1, final LDATE a2)
		{
			return (a1 != null) ? (a2 != null && a1.v == a2.v) : (a2 == null);
		}
		LDATE()
		{
			v = 0;
		}
		LDATE(final LDATE s)
		{
			v = (s != null) ? s.v : 0;
		}
		public static int Difference(LDATE d1, LDATE d2)
		{
			int nd1 = DateToDaysSinceChristmas(d1.year(), d1.month(), d1.day());
			int nd2 = DateToDaysSinceChristmas(d2.year(), d2.month(), d2.day());
			return (nd1 - nd2);
		}
		public static LDATE Plus(LDATE d, int days)
		{
			int nd = DateToDaysSinceChristmas(d.year(), d.month(), d.day());
			return DaysSinceChristmasToDate(nd + days);
		}
		public boolean IsEq(LDATE other)
		{
			return (other != null) ? (v == other.v) : false;
		}
		public int DayOfWeek(boolean sundayIsSeventh)
		{
			// 1/1/1970 - Thu (4)
			int dif = Difference(this, new LDATE(1, 1, 1970));
			int dow = (int)((4 + dif % 7) % 7);
			return !sundayIsSeventh ? dow : ((dow != 0) ? dow : 7);
		}
		LDATE(int day, int month, int year)
		{
			int    shift;
			int    x;
			int    d_ = 0, m_ = 0, y_ = 0;
			if(day == ANY_DATE_VALUE) {
				d_ = ANY_DAYITEM_VALUE;
			}
			else if((day & REL_DATE_MASK) != 0) {
				shift = LoWord(day);
				if(month == -1 && year == -1) {
					v = MakeInt(shift, 0x8000);
					return;
				}
				else {
					if(shift < 0)
						x = (shift <= -31) ? (0x40 | 31) : (0x40 | (-shift));
					else if(shift > 0)
						x = (shift >= 31) ? 31 : shift;
					else
						x = 0;
					d_ = (0x80 | x);
				}
			}
			else if((day & THRSMDAY_DATE_MASK) != 0) {
				shift = LoWord(day);
				if(shift >= 1 && shift <= 31) {
					v = MakeInt(shift, 0x2000);
					return;
				}
				else {
					v = 0;
					return;
				}
			}
			else
				d_ = day;
			if(month == ANY_DATE_VALUE) {
				m_ = ANY_MONITEM_VALUE;
			}
			else if((month & REL_DATE_MASK) != 0) {
				shift = LoWord(month);
				if(shift < 0)
					x = (shift <= -24) ? (0x40 | 24) : (0x40 | (-shift));
				else if(shift > 0)
					x = (shift >= 24) ? 24 : shift;
				else
					x = 0;
				m_ = (0x80 | x);
			}
			else
				m_ = month;
			if(year == ANY_DATE_VALUE) {
				y_ = ANY_YEARITEM_VALUE;
			}
			else if((year & REL_DATE_MASK) != 0) {
				shift = LoWord(year);
				if(shift < 0)
					x = (shift <= -255) ? (0x0400 | 255) : (0x0400 | (-shift));
				else if(shift > 0)
					x = (shift >= 255) ? 255 : shift;
				else
					x = 0;
				y_ = (0x4000 | x);
			}
			else
				y_ = year;
			v = MakeInt((m_ << 8) | d_, y_);
		}
		public int day()  { return LoByte(LoWord(v)); }
		public int month()  { return HiByte(LoWord(v)); }
		public int year()   { return HiWord(v); }
		public String Format(int style)
		{
			return datefmt(day(), month(), year(), style);
		}
		int v;
	}
	private static class DateFmtDescr {
		DateFmtDescr(char div, int ord)
		{
			Div = div;
			Ord = ord;
		}
		char Div;
		int  Ord;
	}
	private static DateFmtDescr DecodeDateFormat(int style)
	{
		style = (style & 0xf);
		switch(style) {
			case DATF_AMERICAN: return new DateFmtDescr('/',0); // DATF_AMERICAN
			case DATF_ANSI: return new DateFmtDescr('.',2); // DATF_ANSI
			case DATF_BRITISH: return new DateFmtDescr('/',1); // DATF_BRITISH
			case DATF_FRENCH: return new DateFmtDescr('/',1); // DATF_FRENCH
			case DATF_GERMAN: return new DateFmtDescr('.',1); // DATF_GERMAN
			case DATF_ITALIAN: return new DateFmtDescr('-',1); // DATF_ITALIAN
			case DATF_JAPAN: return new DateFmtDescr('/',2); // DATF_JAPAN
			case DATF_USA: return new DateFmtDescr('-',0); // DATF_USA
			case DATF_MDY: return new DateFmtDescr('/',0); // DATF_MDY
			case DATF_DMY: return new DateFmtDescr('/',1); // DATF_DMY
			case DATF_YMD: return new DateFmtDescr('/',2); // DATF_YMD
			case DATF_SQL: return new DateFmtDescr('-',2); // DATF_SQL то же, что ISO8601 но с префиксом DATE и в апострофах: DATE 'YYYY-MM-DD'
			case DATF_INTERNET: return new DateFmtDescr(' ',1); // DATF_INTERNET (формальный подход не сработает - структура сложная)
			case DATF_ISO8601: return new DateFmtDescr('-',2);  // DATF_ISO8601
			default: return new DateFmtDescr('-',2);
		}
	}
	public static String datefmt(int day, int mon, int year, int style)
	{
		String result = null;
		//int    div;
		//int    ord;		/* 0 - mm.dd.yy, 1 - dd.mm.yy, 2 - yy.mm.dd */
		int    yr = ((style & DATF_CENTURY) != 0) ? year : year % 100;
		if(style == DATF_SQL) {
			if(day == 0 && mon == 0 && year == 0) {
				year = 1900;
				mon = 1;
				day = 1;
			}
			result = String.format("DATE '%04d-%02d-%02d'", year, mon, day);
		}
		else if(style == DATF_INTERNET) {
			//#define DATF_INTERNET      13  // Wed, 27 Feb 2008
			Calendar cal = new Calendar.Builder().setDate(year, mon, day).build();
			LDATE _dt = new LDATE(day, mon, year);
			int   dow = _dt.DayOfWeek(true); //dayofweek(&_dt, 1); // @v9.7.10 0-->1 в связи с вводом глобальной функции STextConst::Get
			//const char * p_dow_txt[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
			//const char * p_mon_txt[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Seb", "Oct", "Nov", "Dec"};
			//strcat(pBuf, p_dow_txt[dow]);
			//sprintf(pBuf, "%s, %d %s %d", p_dow_txt[dow%7], day, p_mon_txt[(mon >= 12) ? 11 : ((mon < 1) ? 0 : (mon-1))], year);
			if(dow < 1 || dow > 7)
				dow = 1;
			result = String.format("%s, %d %s %d", dow_en_sh[dow-1], day,
					mon_en_sh[(mon >= 12) ? 11 : ((mon < 1) ? 0 : (mon-1))], year);
		}
		else if(day == 0 && mon == 0 && year == 0)
			result = "";
		else {
			int    d_ = 0, m_ = 0, y_ = 0;
			int    shift;
			String sd = "";
			String sm = "";
			String sy = "";
			if((day & 0x20000000) != 0) {
				shift = LoWord(day);
				if(mon == -1 && year == -1) {
					result = "^" + Integer.toString(shift);
					return result;
				}
				else {
					sd = "^" + Integer.toString(shift);
				}
			}
			if((day & 0x80000000) != 0) {
				shift = LoWord(day);
				if(mon == -1 && year == -1) {
					result = "@";
					if(shift > 0) {
						result = result + "+" + Integer.toString(shift);
					}
					else if(shift < 0) {
						result = result + Integer.toString(shift);
					}
					return result;
				}
				else {
					sd = "@";
					if(shift < 0) {
						sd = sd + Integer.toString(shift);
					}
					else if(shift > 0) {
						sd = sd + "+" + Integer.toString(shift);
					}
				}
			}
			else {
				sd = String.format("%02d", day);
			}
			if((mon & 0x80000000) != 0) {
				shift = LoWord(mon);
				sm = "@";
				if(shift < 0) {
					sm = sm + Integer.toString(shift);
				}
				else if(shift > 0) {
					sm = sm + "+" + Integer.toString(shift);
				}
			}
			else {
				sm = String.format("%02d", mon);
			}
			if((year & 0x80000000) != 0) {
				shift = LoWord(year);
				sy = "@";
				if(shift < 0) {
					sy = sy + Integer.toString(shift);
				}
				else if(shift > 0) {
					sy = sy + "+" + Integer.toString(shift);
				}
			}
			else {
				sy = String.format("%02d", ((style & DATF_CENTURY) != 0) ? year : year % 100);
			}
			DateFmtDescr dfd = DecodeDateFormat(style);
			//ord = _decode_date_fmt(style, &div);
			if(dfd.Ord == 0) {
				String temp = sd;
				sd = sm;
				sm = temp;
				//memswap(sd, sm, sizeof(sd));
			}
			else if(dfd.Ord == 2) {
				String temp = sd;
				sd = sy;
				sy = temp;
				//memswap(sd, sy, sizeof(sd));
			}
			{
				//int p = 0;
				//int len = sd.length();
				//memcpy(pBuf+p, sd, len);
				result = sd;
				//p += len;
				if((style & DATF_NODIV) == 0) {
					result = result + dfd.Div;
					//pBuf[p++] = div;
				}
				//len = sm.length();
				result = result + sm;
				//memcpy(pBuf+p, sm, len);
				//p += len;
				if((style & DATF_NODIV) == 0) {
					result = result + dfd.Div;
					//pBuf[p++] = div;
				}
				//len = sstrlen(sy);
				result = result + sy;
				//memcpy(pBuf+p, sy, len);
				//p += len;
				//pBuf[p++] = 0;
			}
		/*
		ord = _decode_date_fmt(style, &div);
		if(ord == 0)
			Exchange((long *)&day, (long *)&mon);
		else if(ord == 2)
			Exchange((long *)&day, (long *)&yr);
		sprintf(pBuf, "%02d%c%02d%c%02d", day, div, mon, div, yr);
		*/
		}
		return result;
	}
	public static String durationfmt(int seconds)
	{
		String result = "";
		if(seconds > 0) {
			int days = seconds / (24 * 3600);
			seconds = seconds % (24 * 3600);
			int h = seconds / 3600;
			seconds = seconds % 3600;
			int m = seconds / 60;
			seconds = seconds % 60;
			if(days > 0) {
				if(GetLen(result) > 0)
					result += " ";
				result += Integer.toString(days) + "days";
			}
			if(h > 0) {
				if(GetLen(result) > 0)
					result += " ";
				result += Integer.toString(h) + "h";
			}
			if(m > 0) {
				if(GetLen(result) > 0)
					result += " ";
				result += Integer.toString(m) + "min";
			}
			if(seconds > 0) {
				if(GetLen(result) > 0)
					result += " ";
				result += Integer.toString(seconds) + "sec";
			}
		}
		return result;
	}
	public static String timefmt(LTIME t, int fmt)
	{
		//char   fs[64];
		String fs = null;
		String result = "";
		if(t.v == 0 && (fmt & TIMF_BLANK) != 0)
			result = "";
		else if((fmt & COMF_SQL) != 0) {
			result = String.format("TIMESTAMP '%04d-%02d-%02d %02d:%02d:%02d.%02d'", 2000, 1, 1, t.hour(), t.minut(), t.sec(), t.hs());
		}
		else {
			final boolean _no_div = ((fmt & TIMF_NODIV) != 0);
			if(_no_div)
				fs = "%02d%02d%02d";
			else if((fmt & TIMF_DOTDIV) != 0)
				fs = "%02d.%02d.%02d";
			else
				fs = "%02d:%02d:%02d";
			switch(fmt & 7) {
				case 2: // TIMF_HM
					fs = fs.substring(0, _no_div ? 8 : 9);
					result = String.format(fs, t.hour(), t.minut());
					break;
				case 3: // TIMF_MS
					fs = fs.substring(0, _no_div ? 8 : 9);
					result = String.format(fs, t.minut(), t.sec());
					break;
				case 4: // TIMF_S
					fs = fs.substring(0, 4);
					result = String.format(fs, t.sec());
					break;
				default: // include 1
					result = String.format(fs, t.hour(), t.minut(), t.sec());
					break;
			}
			if((fmt & TIMF_MSEC) != 0) {
				result += String.format(".%03d", t.hs() * 10);
			}
			if((fmt & TIMF_TIMEZONE) != 0) {
				TimeZone time_zone = TimeZone.getDefault();
				Calendar cal = Calendar.getInstance(time_zone, Locale.getDefault());
				int tz_offs_millis = time_zone.getOffset(cal.getTimeInMillis());
				//
				result += " ";
				if(tz_offs_millis > 0)
					result += "+";
				else
					result += "-";
				result += String.format("%02d%02d", tz_offs_millis / 60000, tz_offs_millis % 60000);
			}
		}
		return result;
		//return _commfmt(fmt, pBuf);
	}
	public static String datetimefmt(LDATETIME dtm, int dtfmt, int tmfmt)
	{
		String result = null;
		int df = SFMTFLAG(dtfmt);
		int tf = SFMTFLAG(tmfmt);
		int df2 = (df & ~(DATF_CENTURY|DATF_NOZERO|DATF_NODIV));
		if(df2 == DATF_SQL || tf == TIMF_SQL) {
			final String format = "%02d:%02d:%04d %02d:%02d:%02d.%02d";
			result = String.format(format, dtm.d.month(), dtm.d.day(), dtm.d.year(), dtm.t.hour(), dtm.t.minut(), dtm.t.sec(), dtm.t.hs());
		}
		else {
			result = datefmt(dtm.d.day(), dtm.d.month(), dtm.d.year(), dtfmt) + ((df2 == DATF_ISO8601) ? "T" : " ") + timefmt(dtm.t, tmfmt);
		}
		return result;
	}

	private static final int strtodatefZero     = 0x0001; // Пустая дата
	private static final int strtodatefDefMon   = 0x0002; // Месяц установлен по умолчанию
	private static final int strtodatefDefYear  = 0x0004; // Год установлен по умолчанию
	private static final int strtodatefRel      = 0x0008; // Дата задана относительно текущего дня +/- количество дней
	private static final int strtodatefRelDay   = 0x0010; // День задан в относительной форме
	private static final int strtodatefRelMon   = 0x0020; // Месяц задан в относительной форме
	private static final int strtodatefRelYear  = 0x0040; // Год задан в относительной форме
	private static final int strtodatefThrsMDay = 0x0080; // Дата задана как начало месяца с порогом в виде дня. Если текущий день менее порога, то - начало предыдущего месяца, иначе - текущего.
	private static final int strtodatefAnyDay   = 0x0100; //
	private static final int strtodatefAnyMon   = 0x0200; //
	private static final int strtodatefAnyYear  = 0x0400; //
	private static final int strtodatefInvalid  = 0x0800; // Дата, извлеченная из строки, инвалидная
	private static final int strtodatefRelAny   = (strtodatefRel|strtodatefRelDay|strtodatefRelMon|strtodatefRelYear|strtodatefThrsMDay); // Маска

	private static final int REL_DATE_MASK      = 0x80000000;
	private static final int THRSMDAY_DATE_MASK = 0x20000000;
	private static final int ANY_DATE_VALUE     = 0x7d0a7d7d;
	private static final int ANY_DAYITEM_VALUE  = 0x7d;
	private static final int ANY_MONITEM_VALUE  = 0x7d;
	private static final int ANY_YEARITEM_VALUE = 0x7d0a;

	public static boolean IsLeapYear(int y)
	{
		return ((y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0));
	}
	private static final int days_per_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	public static int DaysPerMonth(int month, int year)
	{
		//assert(checkirange(month, 1, 12));
		//assert(year > 0);
		int    dpm = 0;
		if(month >= 1 && month <= 12) {
			dpm = days_per_month[month-1];
			if(month == 2 && IsLeapYear(year))
				dpm++;
		}
		return dpm;
	}
	public static LDATE BuildDateByEpoch(long millis)
	{
		LDATETIME _dtm = new LDATETIME(millis);
		return _dtm.d;
		/*
		Calendar cal = Calendar.getInstance();
		cal.setTimeInMillis(millis);
		int d = cal.get(Calendar.DAY_OF_MONTH);
		int m = cal.get(Calendar.MONTH)+1;
		int y = cal.get(Calendar.YEAR);
		return new LDATE(d, m, y);
		 */
	}
	public static LDATE GetCurDate()
	{
		return BuildDateByEpoch(System.currentTimeMillis());
	}
	public static boolean CheckDate(LDATE d)
	{
		return (d != null) ? CheckDate(d.day(), d.month(), d.year()) : false;
	}
	public static boolean CheckDate(int day, int mon, int year)
	{
		int    err = 0;
		if((year & REL_DATE_MASK) == 0 && (year != ANY_DATE_VALUE) && (year < 1801 || year > 2099))
			err = SLERR_INVYEAR;
		else if((mon & REL_DATE_MASK) == 0 && (mon != ANY_DATE_VALUE) && (mon < 1 || mon > 12))
			err = SLERR_INVMONTH;
		else if((day & (REL_DATE_MASK|THRSMDAY_DATE_MASK)) == 0 && (day != ANY_DATE_VALUE)) {
			if(day < 1)
				err = SLERR_INVDAY;
			else if((mon & REL_DATE_MASK) != 0) {
				if(day > 31)
					err = SLERR_INVDAY;
			}
			else if((year & REL_DATE_MASK) != 0) {
				if(day > DaysPerMonth(mon, 2000)) // any leap year
					err = SLERR_INVDAY;
			}
			else if(mon == ANY_MONITEM_VALUE || mon == ANY_DATE_VALUE) {
				if(day > 31)
					err = SLERR_INVDAY;
			}
			else if(day > DaysPerMonth(mon, year))
				err = SLERR_INVDAY;
		}
		return (err == 0);
	}
	//
	// Descr: Константы, идентифицирующие значения, ассоциированные со временем и датой
	// @v11.2.10 @construction
	//
	public static final int timeconstMONTHSHIFT      = 20;
	public static final int timeconstDAYOFWEEKSHIFT  = 40;

	public static final int timeconstYear        =  1;
	public static final int timeconstMonth       =  2;
	public static final int timeconstDayOfMonth  =  3;
	public static final int timeconstDayOfWeek   =  4;
	public static final int timeconstHour        =  5;
	public static final int timeconstMinute      =  6;
	public static final int timeconstSecond      =  7;
	public static final int timeconstMillisecond =  8;
	public static final int timeconstZoneOffset  =  9;
	public static final int timeconstDstOffset   = 10;

	public static final int timeconstMonthJan    = 1+timeconstMONTHSHIFT;
	public static final int timeconstMonthFeb    = 2+timeconstMONTHSHIFT;
	public static final int timeconstMonthMar    = 3+timeconstMONTHSHIFT;
	public static final int timeconstMonthApr    = 4+timeconstMONTHSHIFT;
	public static final int timeconstMonthMay    = 5+timeconstMONTHSHIFT;
	public static final int timeconstMonthJun    = 6+timeconstMONTHSHIFT;
	public static final int timeconstMonthJul    = 7+timeconstMONTHSHIFT;
	public static final int timeconstMonthAug    = 8+timeconstMONTHSHIFT;
	public static final int timeconstMonthSep    = 9+timeconstMONTHSHIFT;
	public static final int timeconstMonthOct    = 10+timeconstMONTHSHIFT;
	public static final int timeconstMonthNov    = 11+timeconstMONTHSHIFT;
	public static final int timeconstMonthDec    = 12+timeconstMONTHSHIFT;

	public static final int timeconstDowMon      = 1+timeconstDAYOFWEEKSHIFT;
	public static final int timeconstDowTue      = 2+timeconstDAYOFWEEKSHIFT;
	public static final int timeconstDowWed      = 3+timeconstDAYOFWEEKSHIFT;
	public static final int timeconstDowThu      = 4+timeconstDAYOFWEEKSHIFT;
	public static final int timeconstDowFri      = 5+timeconstDAYOFWEEKSHIFT;
	public static final int timeconstDowSat      = 6+timeconstDAYOFWEEKSHIFT;
	public static final int timeconstDowSun      = 7+timeconstDAYOFWEEKSHIFT;

	private static int GetDateToken(int cnt, int ord)
	{
		switch(cnt) {
			case 0: return (ord == 0) ? timeconstMonth : ((ord == 1) ? timeconstDayOfMonth : timeconstYear);
			case 1: return (ord > 0) ? timeconstMonth : timeconstDayOfMonth;
			case 2:
			default: return (ord == 2) ? timeconstDayOfMonth : timeconstYear;
		}
	}
	public static double strtodouble(final String buf)
	{
		double result = 0.0;
		if(GetLen(buf) > 0) {
			buf.replace(',', '.');
			try {
				result = Double.parseDouble(buf);
			} catch(NumberFormatException exn) {
				result = 0.0;
			}
		}
		return result;
	}
	public static String formatdouble(double v, int decimals)
	{
		String fmt = "%." + Integer.toString(decimals) + "f";
		return String.format(Locale.US, fmt, v);
	}
	public static @NotNull String FormatCurrency(double val, String currencySymb)
	{
		NumberFormat format = NumberFormat.getCurrencyInstance();
		format.setMaximumFractionDigits(2);
		if(SLib.GetLen(currencySymb) > 0)
			format.setCurrency(Currency.getInstance(currencySymb));
		String result = format.format(val);
		if(result == null)
			result = "";
		return result;
	}
	public static LDATE strtodate(String buf, int style)
	{
		// DATF_DMY

		//
		// @          - текущий день
		// @-2        - текущий день минус два
		// ^10        - если день месяца менее 10, то начало предыдущего месяца, иначе - начало текущего.
		// 11/9/@     - 11/9 текущего года
		// @/@/@-1    - текущая дата год назад
		// @/@-2/2007 - текущий день 2007 года минус два месяца
		//
		// @construction {
		// #w - первый день текущей недели
		// #m - первый день текущего месяца
		// #q - первый день текущего квартала
		// #y - первый день текущего года
		// } @construction
		//
		//const  char * c = pBuf;
		//char   tmp[32];
		//char   zero_buf[32];
		int    i, cnt = 0;
		//int    div;
		//int    ord; // 0 - mm.dd.yy, 1 - dd.mm.yy, 2 - yy.mm.dd
		boolean not_empty_year = false;
		int    d = 0;
		int    m = 0;
		int    y = 0;
		int    plus_d = 0;
		int    plus_m = 0;
		int    plus_y = 0;
		boolean is_first_subst = false;
		long   ret_flags = 0;
		int    _idx = 0;
		final  int _len = GetLen(buf);
		final LDATE  cdate = GetCurDate();
		if(_len == 0) {
			//PTR32(zero_buf)[0] = 0;
			//c = zero_buf;
			buf = "";
			ret_flags |= strtodatefZero;
		}
		else {
			while(_idx < _len && (buf.charAt(_idx) == ' ' || buf.charAt(_idx) == '\t'))
				_idx++;
			if((_idx + 4) <= buf.length() && buf.substring(_idx, 4).equalsIgnoreCase("date")) {
				_idx += 4;
				while(_idx < _len && (buf.charAt(_idx) == ' ' || buf.charAt(_idx) == '\t'))
					_idx++;
				if(buf.charAt(_idx) == '\'')
					_idx++;
			}
			if(_idx < _len && buf.charAt(_idx) != 0) {
				if(buf.charAt(_idx) == '^') {
					_idx++;
					i = 0;
					String tmp = "";
					if(_idx < _len && isdec(buf.charAt(_idx))) {
						tmp += buf.charAt(_idx);
						_idx++;
						if(_idx < _len && isdec(buf.charAt(_idx))) {
							tmp += buf.charAt(_idx);
							_idx++;
						}
					}
					i = Integer.getInteger(tmp);
					if(i >= 1 && i <= 31) {
						d = MakeInt(i, 0x2000);
						m = -1;
						y = -1;
						ret_flags |= strtodatefThrsMDay;
					}
				}
				else {
					//
					// Препроцессинг с целью выяснить не является ли строка датой без разделителей.
					// Такая дата представляет из себя строку из шести символов
					// без всяких пробелов и разделителей.
					//
					int dig_count = 0;
					int year_start = 0;
					int year_end = 0;
					while(dig_count < _len && isdec(buf.charAt(dig_count)))
						dig_count++;
					if(dig_count == 6)
						not_empty_year = true;
					else if(dig_count == 8) {
						not_empty_year = true;
						year_start = (buf.charAt(0) - '0') * 1000;
						year_start += (buf.charAt(1) - '0') * 100;
						year_start += (buf.charAt(2) - '0') * 10;
						year_start += (buf.charAt(3) - '0');
						year_end = (buf.charAt(4) - '0') * 1000;
						year_end += (buf.charAt(5) - '0') * 100;
						year_end += (buf.charAt(6) - '0') * 10;
						year_end += (buf.charAt(7) - '0');
						if(year_start < 1900 || year_start > 2100)
							year_start = 0;
						if(year_end < 1900 || year_end > 2100)
							year_end = 0;
					}
					else {
						//
						// Попытка с ходу засечь дату в формате DATF_ISO8601 (yyyy-mm-dd).
						// Так как по счастливой случайности этот формат не перекрывается никакими иными DATF_XXX
						// форматами, мы может рассмотреть этот вариант отдельно.
						// За одно воспользуемся тем, что количество лидирующих цифр уже подсчитано (dig_count == 4)
						//
						if(dig_count == 4 && style != DATF_ISO8601 && _len > 9) {
							if(buf.charAt(4) == '-' && isdec(buf.charAt(5)) && isdec(buf.charAt(6)) && buf.charAt(7) == '-' && isdec(buf.charAt(8)) && isdec(buf.charAt(9))) {
								style = DATF_ISO8601;
							}
						}
						dig_count = 0;
					}
					//
					//ord = _decode_date_fmt(style, &div);
					DateFmtDescr dfd = DecodeDateFormat(style);
					if(year_start != 0) {
						if(year_end == 0)
							dfd.Ord = 2; // YMD
						else {
							//
							// Если и префикс и суффикс строки может быть трактован как год, то
							// считаем годом то, что ближе к текущей дате.
							//
							int _cy = cdate.year();
							if(Math.abs(_cy - year_start) < Math.abs(_cy - year_end)) // @v10.3.1 @fix (abs(_cy - year_end) < abs(_cy - year_end))-->(abs(_cy - year_start) < abs(_cy - year_end))
								dfd.Ord = 2; // YMD
						}
					}
					for(cnt = 0; cnt < 3; cnt++) {
						//int * p_cur_pos = getnmb(cnt, dfd.Ord, &d, &m, &y);
						int dtok = GetDateToken(cnt, dfd.Ord);
						int cur_value = 0;
						int cur_value_plus = 0;
						if(dig_count > 0) {
							if(dig_count == 8 && dtok == timeconstYear) {
								cur_value = (buf.charAt(_idx) - '0') * 1000;
								_idx++;
								cur_value += (buf.charAt(_idx) - '0') * 100;
								_idx++;
								cur_value += (buf.charAt(_idx) - '0') * 10;
								_idx++;
								cur_value += (buf.charAt(_idx) - '0');
								_idx++;
								//*p_cur_pos = ((*c++) - '0') * 1000;
								//*p_cur_pos += ((*c++) - '0') * 100;
								//*p_cur_pos += ((*c++) - '0') * 10;
								//*p_cur_pos += ((*c++) - '0');
							}
							else if(dig_count >= 2) {
								cur_value = (buf.charAt(_idx) - '0') * 10;
								_idx++;
								cur_value += (buf.charAt(_idx) - '0');
								_idx++;
								//*p_cur_pos = ((*c++) - '0') * 10;
								//*p_cur_pos += ((*c++) - '0');
							}
							else if(dig_count >= 1) {
								cur_value = (buf.charAt(_idx) - '0');
								_idx++;
							}
						}
						else {
							while(_idx < _len && (buf.charAt(_idx) == ' ' || buf.charAt(_idx) == '\t'))
								_idx++;
							if(_idx < _len && buf.charAt(_idx) == '@') {
								is_first_subst = (cnt == 0);
								_idx++;
								int dtok_plus = GetDateToken(cnt, dfd.Ord);
								//int * p_cur_pos_plus = getnmb(cnt, dfd.Ord, &plus_d, &plus_m, &plus_y);
								//*p_cur_pos = -1;
								cur_value_plus = -1;
								if(_idx < _len && (buf.charAt(_idx) == '+' || buf.charAt(_idx) == '-')) {
									int sign = (buf.charAt(_idx) == '-') ? -1 : +1;
									_idx++;
									String tmp = "";
									while(_idx < _len && isdec(buf.charAt(_idx))) {
										tmp = tmp + buf.charAt(_idx);
										_idx++;
									}
									cur_value_plus = satoi(tmp) * sign;
								}
								cur_value = -1;
							}
							else if(_idx < _len && buf.charAt(_idx) == '?') {
								_idx++;
								cur_value = ANY_DATE_VALUE;
								if(dtok == timeconstYear)
									not_empty_year = true;
							}
							else {
								String tmp = "";
								while(_idx < _len && isdec(buf.charAt(_idx))) {
									tmp = tmp + buf.charAt(_idx);
									_idx++;
								}
								cur_value = satoi(tmp);
								if(dtok == timeconstYear && tmp.length() > 0)
									not_empty_year = true;
							}
							//
							switch(dtok) {
								case timeconstDayOfMonth:
									d = cur_value;
									break;
								case timeconstMonth:
									m = cur_value;
									break;
								case timeconstYear:
									y = cur_value;
									break;
							}
							//
							while(_idx < _len && (buf.charAt(_idx) == ' ' || buf.charAt(_idx) == '\t'))
								_idx++;
							char _c = (_idx < _len) ? buf.charAt(_idx) : 0;
							char _c2 = ((_idx + 1) < _len) ? buf.charAt(_idx + 1) : 0;
							if(!(_c == '.' && _c2 != '.') && !(_c == '/' || _c == '-' || _c == '\\'))
								break;
							_idx++;
						}
					}
					if(y != 0 || m != 0 || d != 0) {
						if(is_first_subst) {
							d = MakeInt(plus_d, 0x8000);
							y = -1;
							m = -1;
							ret_flags |= strtodatefRel;
						}
						else {
							if(y == -1) {
								y = MakeInt(plus_y, 0x8000);
								ret_flags |= strtodatefRelYear;
							}
							else if(y == ANY_DATE_VALUE) {
								ret_flags |= strtodatefAnyYear;
							}
							if(m == -1) {
								m = MakeInt(plus_m, 0x8000);
								ret_flags |= strtodatefRelMon;
							}
							else if(m == ANY_DATE_VALUE) {
								ret_flags |= strtodatefAnyMon;
							}
							if(d == -1) {
								d = MakeInt(plus_d, 0x8000);
								ret_flags |= strtodatefRelDay;
							}
							else if(d == ANY_DATE_VALUE) {
								ret_flags |= strtodatefAnyDay;
							}
						}
						if(y == 0) {
							if(!not_empty_year) {
								y = cdate.year();
								ret_flags |= strtodatefDefYear;
							}
							else
								y = 2000;
						}
						if(m == 0) {
							m = cdate.month();
							ret_flags |= strtodatefDefMon;
						}
						if(d == 0)
							d = 1;
						if(y > 0 && y < 100) {
							if(y >= 70) // @v10.8.5 50-->70
								y += 1900;
							else
								y += 2000;
						}
						else if(y >= 200 && y <= 299)
							y = 2000 + (y - 200);
					}
				}
			}
			if(!CheckDate(d, m, y)) {
				d = 0;
				m = 0;
				y = 0;
				ret_flags |= strtodatefInvalid;
			}
		}
		return new LDATE(d, m, y);
	}
	private static boolean checkdeccount(String buf, int startOffset, int decCount)
	{
		for(int i = startOffset; i < decCount; i++) {
			if(!isdec(buf.charAt(i)))
				return false;
		}
		return true;
	}
	private static int _texttodec32(String buf, int startOffset, int len)
	{
		int result = 0;
		char c0 = buf.charAt(startOffset+0);
		char c1 = (len > 1) ? buf.charAt(startOffset+1) : 0;
		char c2 = (len > 2) ? buf.charAt(startOffset+2) : 0;
		char c3 = (len > 3) ? buf.charAt(startOffset+3) : 0;
		char c4 = (len > 4) ? buf.charAt(startOffset+4) : 0;
		switch(len) {
			case 0: result = 0; break;
			case 1: result = c0 - '0'; break;
			case 2: result = (10 * (c0 - '0')) + (c1 - '0'); break;
			case 3: result = (100 * (c0 - '0')) + (10 * (c1 - '0')) + (c2 - '0'); break;
			case 4: result = (1000 * (c0 - '0')) + (100 * (c1 - '0')) + (10 * (c2 - '0')) + (c3 - '0'); break;
			case 5: result = (10000 * (c0 - '0')) + (1000 * (c1 - '0')) + (100 * (c2 - '0')) + (10 * (c3 - '0')) + (c4 - '0'); break;
			default:
				result = 0;
				for(int i = 0; i < len; i++)
					result = (result * 10) + (buf.charAt(startOffset+i) - '0');
				break;
		}
		return result;
	}
	//
	// Descr: Представление времени. Бинарно совместимо с LTIME из slib.h
	//
	public static class LTIME {
		public static boolean ArEq(final LTIME a1, final LTIME a2)
		{
			return (a1 != null) ? (a2 != null && a1.v == a2.v) : (a2 == null);
		}
		LTIME()
		{
			v = 0;
		}
		LTIME(final LTIME s)
		{
			v = (s != null) ? s.v : 0;
		}
		LTIME(int h, int m, int s, int ms)
		{
			v = MakeInt(((s << 8) & 0xff00) | ((ms / 10) & 0x00ff), ((h << 8)&0xff00) | (m&0x00ff));
		}
		int    hour()  { return HiByte(HiWord(v)); }
		int    minut() { return LoByte(HiWord(v)); }
		int    sec() { return HiByte(LoWord(v)); }
		//
		// Descr: Возвращает количество сотых долей секунды в значении this.
		//
		int    hs() { return LoByte(LoWord(v)); }
		int    totalsec() { return (sec() + minut() * 60 + hour() * 3600); }
		//
		// Descr: Устанавливает время в соответствии с количеством секунд, заданных параметром s.
		// Returns:
		//   Количество целых суток, содержащихся в s
		//
		int   settotalsec(int s)
		{
			int inc_dt = s / (3600 * 24);
			v = MakeInt(((s%60)<<8), ((s / 3600) << 8) | (0x00ff & ((s % 3600) / 60)));
			return inc_dt;
		}
		int v;
	}
	public static LTIME strtotime(String buf, int fmt)
	{
		int    ok = 1;
		int    h = 0;
		int    m = 0;
		int    s = 0;
		int    ms = 0;
		int    _len = GetLen(buf);
		if(_len == 0) {
			//PTR32(zero_buf)[0] = 0;
			//c = zero_buf;
			buf = "";
		}
		else if(_len > 0) {
			//final  LDATE  cdate = GetCurDate();
			int _idx = 0;
			while(_idx < _len && (buf.charAt(_idx) == ' ' || buf.charAt(_idx) == '\t'))
				_idx++;
			if((fmt & TIMF_NODIV) != 0) {
				switch(fmt & 7) {
					case TIMF_HMS:
						if(checkdeccount(buf, _idx, 6)) {
							h = _texttodec32(buf, _idx, 2);
							m = _texttodec32(buf, _idx+2, 2);
							s = _texttodec32(buf, _idx+4, 2);
						}
						break;
					case TIMF_HM:
						if(checkdeccount(buf, _idx,4)) {
							h = _texttodec32(buf, _idx,2);
							m = _texttodec32(buf, _idx+2, 2);
						}
						break;
					case TIMF_MS:
						if(checkdeccount(buf, _idx,4)) {
							m = _texttodec32(buf, _idx,2);
							s = _texttodec32(buf, _idx+2, 2);
						}
						break;
					case TIMF_S:
					{
						int   p = 0;
						if(isdec(buf.charAt(_idx+p))) {
							do {
								p++;
							} while(isdec(buf.charAt(_idx+p)));
							s = _texttodec32(buf, _idx, p);
						}
					}
					break;
				}
			}
			else {
				int   p = 0;
				if(_idx < _len && isdec(buf.charAt(_idx))) {
					do { p++; } while((_idx+p) < _len && isdec(buf.charAt(_idx+p)));
					h = _texttodec32(buf, _idx, p);
					char c = buf.charAt(_idx+p);
					if(c == ':' || c == ';' || c == ' ') {
						buf = buf.substring(_idx+p+1);
						_len = buf.length();
						_idx = 0;
						p = 0;
						if(_len > 0 && isdec(buf.charAt(0))) {
							do { p++; } while(p < _len && isdec(buf.charAt(p)));
							m = _texttodec32(buf, 0, p);
							c = buf.charAt(p);
							if(c == ':' || c == ';' || c == ' ') {
								buf = buf.substring(p+1);
								_len = buf.length();
								p = 0;
								if(_len > 0 && isdec(buf.charAt(0))) {
									do { p++; } while(p < _len && isdec(buf.charAt(p)));
									s = _texttodec32(buf, 0, p);
									if(p < _len && buf.charAt(p) == '.') {
										buf = buf.substring(p+1);
										_len = buf.length();
										p = 0;
										if(_len > 0 && isdec(buf.charAt(0))) {
											do { p++; } while(p < _len && isdec(buf.charAt(p)));
											ms = _texttodec32(buf, 0, p);
										}
									}
								}
							}
						}
					}
				}
			}
		}
		return new LTIME(h, m, s, ms);
	}
	public static LDATETIME strtodatetime(String buf, int dtfmt, int tmfmt)
	{
		LDATETIME result = null;
		if(GetLen(buf) > 0) {
			String ds = null;
			String ts = null;
			for(int i = 0; ds == null && i < buf.length(); i++) {
				char c = buf.charAt(i);
				if(c == ' ' || c == 'T') {
					ds = buf.substring(0, i);
					if((i+1) < buf.length())
						ts = buf.substring(i+1, buf.length());
					break;
				}
			}
			if(GetLen(ds) > 0) {
				result = new LDATETIME();
				result.d = strtodate(ds, dtfmt);
				if(GetLen(ts) > 0)
					result.t = strtotime(ts, tmfmt);
			}
		}
		return result;
	}
	public static class LDATETIME {
		public static boolean ArEq(final LDATETIME a1, final LDATETIME a2)
		{
			return (a1 != null) ? (a2 != null && LDATE.ArEq(a1.d, a2.d) && LTIME.ArEq(a1.t, a2.t)) : (a2 == null);
		}
		public static LDATETIME Copy(LDATETIME s)
		{
			return (s != null) ? new LDATETIME(s) : null;
		}
		LDATETIME()
		{
			d = new LDATE();
			t = new LTIME();
		}
		LDATETIME(final LDATETIME s)
		{
			if(s != null) {
				d = new LDATE(s.d);
				t = new LTIME(s.t);
			}
			else {
				d = new LDATE();
				t = new LTIME();
			}
		}
		LDATETIME(LDATE _d, LTIME _t)
		{
			d = new LDATE(_d);
			t = new LTIME(_t);
		}
		LDATETIME(long epochMilliseconds)
		{
			Calendar c = Calendar.getInstance();
			c.setTimeInMillis(epochMilliseconds);
			d = new LDATE(c.get(Calendar.DAY_OF_MONTH), c.get(Calendar.MONTH)+1, c.get(Calendar.YEAR));
			t = new LTIME(c.get(Calendar.HOUR_OF_DAY), c.get(Calendar.MINUTE), c.get(Calendar.SECOND), c.get(Calendar.MILLISECOND));
		}
		LDATE d;
		LTIME t;
	}
	public static LDATETIME plusdatetimesec(LDATETIME dtm1, long plus)
	{
		// dim == 3
		LDATETIME result = null;
		if(dtm1 != null) {
			int days = 0;
			int h = dtm1.t.hour();
			int m = dtm1.t.minut();
			int s = dtm1.t.sec();
			int hs = dtm1.t.hs();
			s += plus;
			//
			// Нормализуем величины в случае, если они выходят за допустимые пределы
			//
			s += (hs / 100);
			if(hs < 0) {
				s--;
				hs = 100 + hs % 100;
			}
			else
				hs %= 100;
			m += (s / 60);
			if(s < 0) {
				m--;
				s = 60 + s % 60;
			}
			else
				s %= 60;
			h += (m / 60);
			if(m < 0) {
				h--;
				m = 60 + m % 60;
			}
			else
				m %= 60;
			days += (h / 24);
			if(h < 0) {
				days--;
				h = 24 + h % 24;
			}
			else
				h %= 24;
			result = new LDATETIME(LDATE.Plus(dtm1.d, days), new LTIME(h, m, s, hs*10));
		}
		return result;
	}
	public static class STimeChunk {
		STimeChunk()
		{
			Start = new LDATETIME();
			Finish = new LDATETIME();
		}
		STimeChunk(LDATETIME start, LDATETIME finish)
		{
			Start = start;
			Finish = finish;
		}
		STimeChunk GetUnionIfIntersected(STimeChunk other)
		{
			STimeChunk result = null;
			if(other != null) {
				STimeChunk is = Intersect(other);
				if(is != null) {
					int sc = Cmp(Start, other.Start);
					int fc = Cmp(Finish, other.Finish);
					result = new STimeChunk();
					if(sc < 0)
						result.Start = Start;
					else
						result.Start = other.Start;
					if(fc > 0)
						result.Finish = Finish;
					else
						result.Finish = other.Finish;
				}
			}
			return result;
		}
		STimeChunk Intersect(STimeChunk test)
		{
			if(Cmp(Start, test.Finish) > 0 || Cmp(Finish, test.Start) < 0) {
				return null;
			}
			else {
				LDATETIME st = (Cmp(Start, test.Start) > 0) ? Start : test.Start;
				LDATETIME fn = (Cmp(Finish, test.Finish) < 0) ? Finish : test.Finish;
				return new STimeChunk(st, fn);
			}
		}
		String Format(int datf, int timf)
		{
			String result = null;
			if(Start != null && CheckDate(Start.d)) {
				if(Finish != null && CheckDate(Finish.d)) {
					if(Start.d.IsEq(Finish.d)) {
						result = Start.d.Format(datf);
						if(Start.t.v == Finish.t.v)
							result += " " + timefmt(Start.t, timf);
						else
							result += " " + timefmt(Start.t, timf) + ".." + timefmt(Finish.t, timf);
					}
					else
						result = datetimefmt(Start, datf, timf) + ".." + datetimefmt(Finish, datf, timf);
				}
				else
					result = datetimefmt(Start, datf, timf) + "..";
			}
			else if(Finish != null && CheckDate(Finish.d)) {
				result = ".." + datetimefmt(Finish, datf, timf);
			}
			else
				result = "";
			return result;
		}
		LDATETIME Start;
		LDATETIME Finish;
	}
	public static class STimeChunkArray extends ArrayList<SLib.STimeChunk> {
		STimeChunkArray()
		{
			super();
		}
		private STimeChunkArray Helper_Intersect(final STimeChunk chunk)
		{
			STimeChunkArray result = null;
			if(chunk != null) {
				for(int j = 0; j < size(); j++) {
					final STimeChunk item = get(j);
					STimeChunk sect = chunk.Intersect(item);
					if(sect != null) {
						if(result == null)
							result = new STimeChunkArray();
						result.add(sect);
					}
				}
			}
			return result;
		}
		STimeChunkArray Intersect(STimeChunk chunk)
		{
			STimeChunkArray result = Helper_Intersect(chunk);
			//result.Sort();
			return result;
		}
		STimeChunkArray Intersect(final STimeChunkArray list)
		{
			STimeChunkArray result = null;
			for(int i = 0; i < list.size(); i++) {
				STimeChunkArray temp_result = Helper_Intersect(list.get(i));
				if(temp_result != null) {
					if(result == null)
						result = temp_result;
					else {
						for(int j = 0; j < temp_result.size(); j++) {
							STimeChunk inner_item = temp_result.get(j);
							if(inner_item != null)
								result.add(inner_item);
						}
					}
				}
			}
			return result;
		}
		//
		// Descr: Формирует список rResult состоящий из объединения временных отрезков this со списком отрезков pList.
		// Note: Функция не пытается объединять пересекающиеся элементы this но только лишь соединяет this с pList.
		//
		STimeChunkArray Union(STimeChunkArray list)
		{
			STimeChunkArray result = null;
			if(list != null) {
				for(int i = 0; i < list.size(); i++) {
					STimeChunk item = list.get(i);
					if(item != null) {
						if(result == null) {
							result = Union(item);
						}
						else {
							result = result.Union(item);
						}
					}
				}
			}
			else
				result = this;
			return result;
		}
		//
		// Descr: Формирует список rResult состоящий из объединения временных отрезков this с отрезком rChunk.
		// Note: Функция не пытается объединять пересекающиеся элементы this но только лишь соединяет this с rChunk.
		//
		STimeChunkArray Union(STimeChunk chunk)
		{
			STimeChunkArray result = null;
			int    ok = 1;
			if(size() == 0) {
				result = new STimeChunkArray();
				result.add(chunk);
			}
			else {
				STimeChunk temp_chunk = chunk;
				Vector<Integer> idx_list_to_exclude = new Vector<>();
				{
					for(int i = 0; i < size(); i++) {
						STimeChunk item = get(i);
						STimeChunk ur = item.GetUnionIfIntersected(chunk);
						if(ur != null) {
							temp_chunk = ur;
							idx_list_to_exclude.add(i);
						}
					}
				}
				{
					result = new STimeChunkArray();
					for(int i = 0; i < size(); i++) {
						if(!idx_list_to_exclude.contains(i)) {
							STimeChunk item = get(i);
							result.add(item);
						}
					}
					result.add(temp_chunk);
				}
			}
			return result;
		}
	}
	public static int Cmp(LDATE t1, LDATE t2)
	{
		if(t1 == null)
			return (t2 == null) ? 0 : -1;
		else if(t2 == null)
			return 1;
		else if(t1.v < t2.v)
			return -1;
		else if(t1.v > t2.v)
			return 1;
		else
			return 0;
	}
	public static int Cmp(LTIME t1, LTIME t2)
	{
		if(t1 == null)
			return (t2 == null) ? 0 : -1;
		else if(t2 == null)
			return 1;
		else if(t1.v < t2.v)
			return -1;
		else if(t1.v > t2.v)
			return 1;
		else
			return 0;
	}
	public static int Cmp(LDATETIME t1, LDATETIME t2)
	{
		if(t1 == null)
			return (t2 == null) ? 0 : -1;
		else if(t2 == null)
			return 1;
		else {
			int r = Cmp(t1.d, t2.d);
			return (r == 0) ? Cmp(t1.t, t2.t) : r;
		}
	}
	public static boolean IsNumeric(String text)
	{
		try {
			Double.parseDouble(text);
			return true;
		} catch(NumberFormatException e) {
			return false;
		}
	}
	public static UUID strtouuid(String text)
	{
		UUID result = null;
		if(GetLen(text) > 0) {
			try {
				result = UUID.fromString(text);
			} catch(IllegalArgumentException exn) {
				result = null;
			}
		}
		return result;
	}
	//

	//
	// Descr: Алгоритмы расчета контрольной цифры
	//
	public static final int SCHKDIGALG_BARCODE = 1; // Розничные штрихкоды (EAN, UPC)
	public static final int SCHKDIGALG_LUHN    = 2; // Алгоритм Луна
	public static final int SCHKDIGALG_RUINN   = 3; // Контрольная цифра ИНН (Россия)
	public static final int SCHKDIGALG_RUOKATO = 4; // Контрольная цифра ОКАТО (Россия)
	public static final int SCHKDIGALG_RUSNILS = 5; // Контрольная цифра СНИЛС (Россия)
	public static final int SCHKDIGALG_TEST    = 0x80000000; // Флаг, предписывающий функции SCalcCheckDigit проверить последовательность на
		// предмет соответствия контрольной цифре, содержащейся в ней.

	public static int SCalcCheckDigit(int alg, final byte [] input, int inputLen)
	{
		int    cd = 0;
		if(input != null && inputLen > 0) {
			int     len = 0;
			int     i;
			byte [] code = new byte[128];
			for(i = 0; i < inputLen; i++) {
				final byte c = input[i];
				if(isdec(c)) {
					if(len >= code.length)
						break;
					code[len++] = c;
				}
				else if(c != '-' && c != ' ')
					break;
			}
			if(len > 0) {
				final int _alg = (alg & ~SCHKDIGALG_TEST);
				final boolean _do_check = ((alg & SCHKDIGALG_TEST) != 0);
				if(_alg == SCHKDIGALG_BARCODE) {
					int    c = 0, c1 = 0, c2 = 0;
					final  int _len = _do_check ? (len-1) : len;
					for(i = 0; i < _len; i++) {
						if((i % 2) == 0)
							c1 += (code[_len-i-1] - '0');
						else
							c2 += (code[_len-i-1] - '0');
					}
					c = c1 * 3 + c2;
					cd = '0' + ((c % 10) != 0 ? (10 - c % 10) : 0);
					if(_do_check)
						cd = (cd == code[len-1]) ? 1 : 0;
				}
				else if(_alg == SCHKDIGALG_LUHN) {
					/*
					// Num[1..N] — номер карты, Num[N] — контрольная цифра.
					sum = 0
					for i = 1 to N-1 do
						p = Num[N-i]
						if(i mod 2 <> 0) then
							p = 2*p
							if(p > 9) then
								p = p - 9
							end if
						end if
						sum = sum + p
					next i
					//дополнение до 10
					sum = 10 - (sum mod 10)
					if(sum == 10) then
						sum = 0
					end if
					Num[N] = sum
					*/
					int    s = 0;
					final int _len = _do_check ? (len-1) : len;
					for(i = 0; i < _len; i++) {
						int    p = (code[_len - i - 1] - '0');
						if((i & 1) == 0) {
							p <<= 1; // *2
							if(p > 9)
								p -= 9;
						}
						s += p;
					}
					s = 10 - (s % 10);
					if(s == 10)
						s = 0;
					cd = '0' + s;
					if(_do_check)
						cd = (cd == code[len-1]) ? 1 : 0;
				}
				else if(_alg == SCHKDIGALG_RUINN) {
					//int CheckINN(const char * pCode)
					{
						int    r = 1;
						if((_do_check && len == 10) || (!_do_check && len == 9)) {
							final int w[] = {2,4,10,3,5,9,4,6,8,0};
							int   sum = 0;
							for(i = 0; i < 9; i++) {
								int   p = (code[i] - '0');
								sum += (w[i] * p);
							}
							cd = '0' + (sum % 11) % 10;
							if(_do_check) {
								cd = (code[9] == cd) ? 1 : 0;
							}
						}
						else if((_do_check && len == 12) || (!_do_check && len == 11)) {
							if(_do_check) {
								final int w1[] = {7,2,4,10, 3,5,9,4,6,8,0};
								final int w2[] = {3,7,2, 4,10,3,5,9,4,6,8,0};
								int   sum1 = 0;
								int   sum2 = 0;
								for(i = 0; i < 11; i++) {
									int   p = (code[i] - '0');
									sum1 += (w1[i] * p);
								}
								for(i = 0; i < 12; i++) {
									int   p = (code[i] - '0');
									sum2 += (w2[i] * p);
								}
								int    cd1 = (sum1 % 11) % 10;
								int    cd2 = (sum2 % 11) % 10;
								cd = ((code[10]-'0') == cd1 && (code[11]-'0') == cd2) ? 1 : 0;
							}
							else {
								cd = -1;
							}
						}
						else
							cd = 0;
					}
				}
				else if(_alg == SCHKDIGALG_RUOKATO) {
				}
				else if(_alg == SCHKDIGALG_RUSNILS) {
				}
			}
		}
		return cd;
	}
	public static byte [] SUpceToUpca(final byte [] upce)
	{
		byte [] upca = new byte [32];
		//char   code[32];
		//char   dest[32];
		//STRNSCPY(code, pUpce);
		int    last = upce[6] - '0';
		for(int i = 0; i < 12; i++)
			upca[i] = '0';
		upca[11] = 0;
		upca[0] = upce[0];
		if(last == 0 || last == 1 || last == 2) {
			upca[1] = upce[1];
			upca[2] = upce[2];
			upca[3] = upce[6];

			upca[8] = upce[3];
			upca[9] = upce[4];
			upca[10] = upce[5];
		}
		else if(last == 3) {
			upca[1] = upce[1];
			upca[2] = upce[2];
			upca[3] = upce[3];

			upca[9] = upce[4];
			upca[10] = upce[5];
		}
		else if(last == 4) {
			upca[1] = upce[1];
			upca[2] = upce[2];
			upca[3] = upce[3];
			upca[4] = upce[4];

			upca[10] = upce[5];
		}
		else { // last = 5..9
			upca[1] = upce[1];
			upca[2] = upce[2];
			upca[3] = upce[3];
			upca[4] = upce[4];
			upca[5] = upce[5];

			upca[10] = upce[6];
		}
		return upca;
	}
	public static int SCalcBarcodeCheckDigitL(final byte [] barcode, int len)
	{
		int    cd = 0;
		if(barcode != null && len > 0) {
			if(len == 7 && barcode[0] == '0') {
				byte [] code = new byte[64];
				for(int i = 0; i < len; i++)
					code[i] = barcode[i];
				code[len] = 0;
				code = SUpceToUpca(code);
				len = (new String(code)).length();
				cd = SCalcCheckDigit(SCHKDIGALG_BARCODE, code, len);
				cd = isdec((byte)cd) ? (cd - '0') : 0;
			}
			else {
				cd = SCalcCheckDigit(SCHKDIGALG_BARCODE, barcode, len);
				cd = isdec((byte)cd) ? (cd - '0') : 0;
			}
		}
		return cd;
	}
	//
	public static class LAssoc {
		public LAssoc()
		{
			Key = 0;
			Value = 0;
		}
		public LAssoc(int k, int v)
		{
			Key = k;
			Value = v;
		}
		boolean Read(InputStream stream)
		{
			return false;
		}
		int Key;
		int Value;
	}
	public static class IntToStrAssoc {
		public IntToStrAssoc()
		{
			Key = 0;
			Value = null;
		}
		public IntToStrAssoc(int k, String value)
		{
			Key = k;
			Value = value;
		}
		int Key;
		String Value;
	}

	private static IntToStrAssoc [] UriSchemeTab = {
		new IntToStrAssoc(uripprotUnkn,         "" ),          // #0
		new IntToStrAssoc(uripprotHttp,         "http" ),      // #1
		new IntToStrAssoc(uripprotHttps,        "https" ),     // #2
		new IntToStrAssoc(uripprotFtp,          "ftp" ),       // #3
		new IntToStrAssoc(uripprotGopher,		 "gopher" ),    // #4
		new IntToStrAssoc(uripprotMailto,		 "mailto" ),    // #5
		new IntToStrAssoc(uripprotNews,		 "news" ),      // #6
		new IntToStrAssoc(uripprotNntp,         "nntp" ),      // #7
		new IntToStrAssoc(uripprotIrc,          "irc" ),       // #8
		new IntToStrAssoc(uripprotProspero,     "prospero" ),  // #9
		new IntToStrAssoc(uripprotTelnet,		 "telnet" ),  // #10
		new IntToStrAssoc(uripprotWais,		 "wais" ),    // #11
		new IntToStrAssoc(uripprotXmpp,		 "xmpp" ),    // #12
		new IntToStrAssoc(uripprotFile,         "file" ),    // #13
		new IntToStrAssoc(uripprotData,         "data" ),    // #14
		new IntToStrAssoc(uripprotSvn,          "svn" ),     // #15
		new IntToStrAssoc(uripprotSocks4,		 "socks4" ),  // #16
		new IntToStrAssoc(uripprotSocks5,		 "socks5" ),  // #17
		new IntToStrAssoc(uripprotSMTP,		 "smtp" ),    // #18
		new IntToStrAssoc(uripprotSMTPS,		 "smtps" ),   // #19
		new IntToStrAssoc(uripprotPOP3,		 "pop3" ),    // #20
		new IntToStrAssoc(uripprotPOP3S,		 "pop3s" ),   // #21
		new IntToStrAssoc(uripprotIMAP,		 "imap" ),    // #22
		new IntToStrAssoc(uripprotIMAPS,		 "imaps" ),   // #23
		new IntToStrAssoc(uripprotFtps,		 "ftps" ),    // #24
		new IntToStrAssoc(uripprotTFtp,		 "tftp" ),    // #25
		new IntToStrAssoc(uripprotDict,		 "dict" ),    // #26
		new IntToStrAssoc(uripprotSSH,          "ssh" ),     // #27
		new IntToStrAssoc(uripprotSMB,          "smb" ),     // #28
		new IntToStrAssoc(uripprotSMBS,         "smbs" ),    // #29
		new IntToStrAssoc(uripprotRTSP,         "rtsp" ),    // #30
		new IntToStrAssoc(uripprotRTMP,         "rtmp" ),    // #31
		new IntToStrAssoc(uripprotRTMPT,        "rtmpt" ),   // #32
		new IntToStrAssoc(uripprotRTMPS,        "rtmps" ),   // #33
		new IntToStrAssoc(uripprotLDAP,         "ldap" ),    // #34
		new IntToStrAssoc(uripprotLDAPS,        "ldaps" ),   // #35
		new IntToStrAssoc(uripprotMailFrom,     "mailfrom" ), // #36 fixion
		new IntToStrAssoc(uripprot_p_PapyrusServer, "" ),         // #37 private PapyrusServer
		new IntToStrAssoc(uripprotAMQP,         "amqp" ),     // #38
		new IntToStrAssoc(uripprotAMQPS,        "amqps" ),    // #39
		new IntToStrAssoc(uripprot_p_MYSQL,     "mysql" ),    // #40 private // @v10.9.2
		new IntToStrAssoc(uripprot_p_SQLITE,    "sqlite" ),   // #41 private // @v10.9.2
		new IntToStrAssoc(uripprot_p_ORACLE,    "oracle" ),   // #42 private // @v10.9.2
		new IntToStrAssoc(uripprotGit,          "git" ),      // #43 // @v11.1.11
	};
	//
	public static class Margin {
		Margin()
		{
			Left = 0;
			Top = 0;
			Right = 0;
			Bottom = 0;
		}
		Margin(int all)
		{
			Left = all;
			Top = all;
			Right = all;
			Bottom = all;
		}
		Margin(int left, int top, int right, int bottom)
		{
			Left = left;
			Top = top;
			Right = right;
			Bottom = bottom;
		}
		int Left;
		int Top;
		int Right;
		int Bottom;
	}
	//
	static int GetUriSchemeId(String uriScheme)
	{
		int result = uripprotUnkn;
		for(int i = 0; i < UriSchemeTab.length; i++) {
			if(UriSchemeTab[i].Value.equalsIgnoreCase(uriScheme))
				return UriSchemeTab[i].Key;
		}
		return result;
	}
	static interface EventHandler {
		public Object HandleEvent(int cmd, Object srcObj, Object subj);
	}
	static abstract class App extends Application implements EventHandler {
		private int CurrentLang; // Текущий выбранный натуральный язык. 0 - default
		protected int      LastError;
		protected String   LastErrMsg;
		protected String   AddInfo;
		protected StrStore _StrStor;
		public void SetCurrentLang(int lang)
		{
			CurrentLang = lang;
		}
		public String GetApplicationVersionText()
		{
			try {
				PackageManager pm = getPackageManager();
				PackageInfo pi = pm.getPackageInfo(getPackageName(), 0);
				return pi.versionName;
			}
			catch(PackageManager.NameNotFoundException exn) {
				return "App not installed!";
			}
		}
		public int GetApplicationVersionCode()
		{
			try {
				PackageManager pm = getPackageManager();
				PackageInfo pi = pm.getPackageInfo(getPackageName(), 0);
				return pi.versionCode;
			}
			catch(PackageManager.NameNotFoundException exn) {
				return 0;
			}
		}
		public int GetCurrentLang() { return CurrentLang; }
		@Override public void onCreate()
		{
			super.onCreate();
			_StrStor = new StrStore();
			try {
				InputStream stream = getResources().openRawResource(R.raw.ppstr2);
				if(_StrStor.Load(stream)) {
					int [] fn_list = { R.raw.ppstr2_de, R.raw.ppstr2_en, R.raw.ppstr2_es, R.raw.ppstr2_nl, R.raw.ppstr2_pt };
					for(int i = 0; i < fn_list.length; i++) {
						InputStream stream2 = getResources().openRawResource(fn_list[i]);
						_StrStor.Load(stream2);
					}
					_StrStor.IndexLangStrColl();
				}
				else
					_StrStor = null;
				HandleEvent(EV_CREATE, this, null);
			} catch(StyloQException exn) {
				exn.printStackTrace();
			}
		}
		@Override public void onTerminate()
		{
			HandleEvent(EV_TERMINTATE, this, null);
			super.onTerminate();
		}
		public static String LoadString(Context ctx, int resId)
		{
			String out_msg = "";
			try {
				out_msg = ctx.getResources().getString(resId);
			}
			catch(Exception exn) {
				exn.printStackTrace();
				out_msg = "[Error of loading resource id=" + resId + "]: " + exn.getMessage();
			}
			return out_msg;
		}
		public static String LoadString(Context context, int resId, String addInfo)
		{
			String fmt = LoadString(context, resId);
			return String.format(fmt, addInfo);
		}
		public void SetLastError(int errCode, String errMsg, String addInfo)
		{
			LastError = errCode;
			LastErrMsg = errMsg;
			AddInfo = addInfo;
		}
		public void SetLastError(int err, String addInfo)
		{
			LastError = err;
			AddInfo = addInfo;
		}
		public void SetLastError(StyloQException exn)
		{
			if(exn != null) {
				LastError = exn.GetErrCode();
				AddInfo = exn.GetAddedInfo();
			}
		}
		public int GetLastError() { return LastError; }
		public void SetLastErrorMsg(String errMsg)
		{
			SetLastErrorMsg(errMsg, "");
		}
		public void SetLastErrorMsg(String errMsg, String addInfo)
		{
			LastErrMsg = errMsg;
			SetLastError(0, addInfo);
		}
		public String GetErrorText(int errCode, String addInfo)
		{
			return GetErrorText(0, errCode, addInfo);
		}
		public String GetErrorText(int msgGroup, int errCode, String addInfo)
		{
			String text = null;
			if(errCode != 0) {
				if(_StrStor != null) {
					String pre_msg = _StrStor.GetString(CurrentLang, (msgGroup == 0) ? ppstr2.PPSTR_ERROR : msgGroup, errCode);
					if(pre_msg != null) {
						text = (addInfo != null) ? String.format(pre_msg, addInfo) : pre_msg;
					}
					else
						text = "Error of loading resource text" + " " + errCode;
				}
				else {
					text = "(StringStore is not loaded)" + " ErrCode=" + errCode;
					if(GetLen(addInfo) > 0)
						text = text + " - " + addInfo;
				}
			}
			return text;
		}
		public static void SetLastError(Context ctx, int errCode, String msg, String addInfo)
		{
			StyloQApp app_ctx = SLib.SlActivity.GetAppCtx(ctx);
			if(app_ctx != null) {
				if(GetLen(msg) > 0)
					app_ctx.SetLastErrorMsg(msg, addInfo);
				else if(errCode != 0) {
					msg = app_ctx.GetErrorText(errCode, addInfo);
					app_ctx.SetLastError(errCode, addInfo);
				}
			}
		}
		public String GetLastErrMessage(Context ctx)
		{
			String msg = "";
			if(LastError != 0 && ctx != null) {
				// @v11.4.5 msg = StyloQApp.LoadString(ctx, LastError, AddInfo);
				msg = GetErrorText(LastError, AddInfo); // @v11.4.5
			}
			else if(LastErrMsg != null && LastErrMsg.length() > 0) {
				msg = (AddInfo != null) ? String.format(LastErrMsg, AddInfo) : LastErrMsg;
			}
			LastErrMsg = "";
			LastError  = 0;
			AddInfo    = "";
			return msg;
		}
		public static String GetLastErrorMessage(Context ctx)
		{
			StyloQApp app_ctx = SLib.SlActivity.GetAppCtx(ctx);
			return (app_ctx != null) ? app_ctx.GetLastErrMessage(ctx) : "";
		}
	}
	public static class RecyclerListViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener {
		private RecyclerListAdapter Adapter;

		RecyclerListViewHolder(View view, RecyclerListAdapter adapter)
		{
			super(view);
			Adapter = adapter;
		}
		@Override public void onClick(View view)
		{
			SlActivity activity = null;
			View v = this.itemView;
			ViewParent iter_view = v.getParent();
			while(iter_view != null) {
				if(iter_view instanceof RecyclerView) {
					Context ctx = ((RecyclerView)iter_view).getContext();
					if(ctx != null && ctx instanceof SlActivity)
						activity = (SlActivity)ctx;
					break;
				}
				else
					iter_view = iter_view.getParent();
			}
			EventHandler eh = Adapter.GetEventHandler(activity);
			if(eh != null) {
				ListViewEvent ev_subj = new ListViewEvent();
				ev_subj.RvHolder = this;
				ev_subj.ItemIdx = this.getLayoutPosition();
				ev_subj.ItemView = view;
				eh.HandleEvent(EV_LISTVIEWITEMCLK, Adapter, ev_subj);
			}
		}
	}
	public static void SetupRecyclerListViewHolderAsClickListener(RecyclerView.ViewHolder viewHolder, View itemView, int ctlRcId)
	{
		if(viewHolder != null && itemView != null && viewHolder instanceof RecyclerListViewHolder) {
			View v = itemView.findViewById(ctlRcId);
			if(v != null)
				v.setOnClickListener((SLib.RecyclerListViewHolder)viewHolder);
		}
	}
	public static class RecyclerListViewClickListener implements View.OnClickListener, View.OnLongClickListener {
		private int Index;
		private final LayoutInflater Inflater;
		private RecyclerListAdapter Adapter;
		private RecyclerListViewHolder Holder;
		public RecyclerListViewClickListener(LayoutInflater inflater, RecyclerListAdapter adapter, RecyclerListViewHolder holder, int idx)
		{
			Inflater = inflater;
			Adapter = adapter;
			Holder = holder;
			Index = idx;
		}
		@Override public void onClick(View v)
		{
			EventHandler eh = Adapter.GetEventHandler(Inflater.getContext());
			if(eh != null) {
				ListViewEvent ev_subj = new ListViewEvent();
				ev_subj.RvHolder = Holder;
				ev_subj.ItemIdx = Index;
				ev_subj.ItemView = v;
				eh.HandleEvent(EV_LISTVIEWITEMCLK, Adapter, ev_subj);
			}
		}
		@Override public boolean onLongClick(View v)
		{
			boolean result = false;
			EventHandler eh = Adapter.GetEventHandler(Inflater.getContext());
			if(eh != null) {
				ListViewEvent ev_subj = new ListViewEvent();
				ev_subj.RvHolder = Holder;
				ev_subj.ItemIdx = Index;
				ev_subj.ItemView = v;
				eh.HandleEvent(EV_LISTVIEWITEMLONGCLK, null, ev_subj);
				result = true;
			}
			return result;
		}
	}
	public static class RecyclerListAdapter extends RecyclerView.Adapter<RecyclerListViewHolder> {
		private int RcId;
		private int ListRcId;
		private int FocusedIdx; // Если (>= 0 && < getCount()), то управляющий класс может отобразать специальным образом
		private LinearLayout _Lo;
		private final LayoutInflater Inflater;
		private EventHandler EventReceiver; // Если !null, то методы класса проверяют, чтобы объект был
			// адекватным получателем сообщений (SlActivity, SlDialog etc) и отправляет сообщения ему.
			// Если null, то сообщения отправляются в контекст (если он является SlActivity)
		RecyclerListAdapter(Context ctx, EventHandler eventReceiver, int listRcId, int rcId)
		{
			super();
			EventReceiver = eventReceiver;
			RcId = rcId;
			ListRcId = listRcId;
			FocusedIdx = -1;
			Inflater = LayoutInflater.from(ctx);
		}
		RecyclerListAdapter(Context ctx, EventHandler eventReceiver, int listRcId, LinearLayout lo)
		{
			super();
			EventReceiver = eventReceiver;
			RcId = 0;
			ListRcId = listRcId;
			FocusedIdx = -1;
			_Lo = lo;
			Inflater = LayoutInflater.from(ctx);
		}
		public EventHandler GetEventHandler(Context ctx)
		{
			return (EventReceiver != null) ? EventReceiver : ((ctx instanceof SlActivity) ? (SlActivity)ctx : null);
		}
		public int GetRcId()
		{
			return RcId;
		}
		public int GetListRcId()
		{
			return ListRcId;
		}
		public final LayoutInflater GetLayoutInflater() { return Inflater; }
		@Override @NonNull public RecyclerListViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
		{
			RecyclerListViewHolder holder = null;
			View view = null;
			Context ctx = Inflater.getContext();
			if(ctx != null) {
				EventHandler eh = GetEventHandler(ctx);
				if(RcId > 0) {
					View root_view = parent;//parent.getRootView();
					if(root_view != null && root_view instanceof ViewGroup) {
						view = Inflater.inflate(RcId, (ViewGroup) root_view, false);
						holder = new RecyclerListViewHolder(view, this);
						if(eh != null) {
							ListViewEvent ev_subj = new ListViewEvent();
							ev_subj.RvHolder = holder;
							ev_subj.ItemView = view;
							ev_subj.ItemIdx = viewType;
							Object ev_result = eh.HandleEvent(EV_CREATEVIEWHOLDER, this, ev_subj);
						}
					}
				}
				else if(eh != null) {
					ListViewEvent ev_subj = new ListViewEvent();
					ev_subj.RvHolder = null;
					ev_subj.ItemView = parent;
					ev_subj.ItemIdx = viewType;
					Object ev_result = eh.HandleEvent(EV_CREATEVIEWHOLDER, this, ev_subj);
					holder = (ev_result != null && ev_result instanceof RecyclerListViewHolder) ? (RecyclerListViewHolder)ev_result : null;
				}
			}
			return holder;
		}
		@Override public void onBindViewHolder(@NonNull RecyclerListViewHolder holder, @SuppressLint("RecyclerView") int position)
		{
			Context ctx = (_Lo != null) ? _Lo.getContext() : Inflater.getContext();
			if(ctx != null) {
				EventHandler eh = GetEventHandler(ctx);
				if(eh != null) {
					ListViewEvent ev_subj = new ListViewEvent();
					ev_subj.RvHolder = holder;
					ev_subj.ItemIdx = position;
					eh.HandleEvent(EV_GETLISTITEMVIEW, this, ev_subj);
				}
				//
				if(_Lo == null) { // @debug
					RecyclerListViewClickListener listener = new RecyclerListViewClickListener(Inflater, this, holder, position);
					holder.itemView.setOnClickListener(listener);
					holder.itemView.setOnLongClickListener(listener);
				}
			}
		}
		@Override public int getItemCount()
		{
			int    result = 0;
			Context ctx = (_Lo != null) ? _Lo.getContext() : Inflater.getContext();
			if(ctx != null) {
				EventHandler eh = GetEventHandler(ctx);
				if(eh != null) {
					Object ret_obj = eh.HandleEvent(EV_LISTVIEWCOUNT, this, null);
					if(ret_obj != null)
						if(ret_obj instanceof Integer)
							result = (Integer)ret_obj;
						else if(ret_obj instanceof Long)
							result = ((Long)ret_obj).intValue();
				}
			}
			return result;
		}
		public void SetFocusedIndex(int idx) { FocusedIdx = idx; }
		public int  GetFocusedIndex() { return FocusedIdx; }
	}
	public static class FragmentAdapter extends FragmentStateAdapter {
		private SlActivity _Activity;
		public FragmentAdapter(SlActivity activity)
		{
			super(activity);
			_Activity = activity;
		}
		@Override @NonNull public androidx.fragment.app.Fragment createFragment(int position)
		{
			androidx.fragment.app.Fragment result = null;
			if(_Activity != null) {
				Object ret_obj = _Activity.HandleEvent(EV_CREATEFRAGMENT, this, new Integer(position));
				if(ret_obj instanceof androidx.fragment.app.Fragment)
					result = (androidx.fragment.app.Fragment)ret_obj;
			}
			return result;
		}
		@Override public int getItemCount()
		{
			int    result = 0;
			if(_Activity != null) {
				Object ret_obj = _Activity.HandleEvent(EV_LISTVIEWCOUNT, this, null);
				if(ret_obj instanceof Integer)
					result = (Integer)ret_obj;
				else if(ret_obj instanceof Long)
					result = ((Long)ret_obj).intValue();
			}
			return result;
		}
	}
	public static class InternalArrayAdapter extends ArrayAdapter {
		protected int RcId;
		private int FocusedIdx;
		InternalArrayAdapter(Context ctx, int rcId, ArrayList data)
		{
			super(ctx, rcId, data);
			RcId = rcId;
			FocusedIdx = -1;
		}
		@Override public View getView(int position, View convertView, ViewGroup parent)
		{
			// Get the data item for this position
			Object item = (Object)getItem(position);
			Context ctx = parent.getContext();
			if(item != null && ctx != null && ctx instanceof SlActivity) {
				SlActivity activity = (SlActivity)parent.getContext();
				// Check if an existing view is being reused, otherwise inflate the view
				if(convertView == null)
					convertView = LayoutInflater.from(getContext()).inflate(RcId, parent, false);
				if(convertView != null) {
					ListViewEvent ev_subj = new ListViewEvent();
					ev_subj.ItemIdx = position;
					ev_subj.ItemObj = item;
					ev_subj.ItemView = convertView;
					//ev_subj.ParentView = parent;
					Object he_result = activity.HandleEvent(EV_GETLISTITEMVIEW, parent, ev_subj);
					if(he_result instanceof View)
						return (View)he_result;
				}
			}
			return convertView; // Return the completed view to render on screen
		}
		public void SetFocusedIndex(int idx) { FocusedIdx = idx; }
		public int  GetFocusedIndex() { return FocusedIdx; }
	}
	/*private static class ListAdapter extends ArrayAdapter {
		//private static final int RcId = R.layout.face_list_item;
		private int RcId;
		ListAdapter(Context ctx, int rcId, ArrayList data)
		{
			super(ctx, rcId, data);
			RcId = rcId;
		}
		@Override public View getView(int position, View convertView, ViewGroup parent)
		{
			// Get the data item for this position
			Object item = (Object)getItem(position);
			Context ctx = parent.getContext();
			if(item != null && ctx != null && ctx instanceof SlActivity) {
				SlActivity activity = (SlActivity)parent.getContext();
				// Check if an existing view is being reused, otherwise inflate the view
				if(convertView == null)
					convertView = LayoutInflater.from(getContext()).inflate(RcId, parent, false);
				if(convertView != null) {
					ListViewEvent ev_subj = new ListViewEvent();
					ev_subj.ItemIdx = position;
					ev_subj.ItemObj = item;
					ev_subj.ItemView = convertView;
					//ev_subj.ParentView = parent;
					Object he_result = activity.HandleEvent(EV_GETLISTITEMVIEW, parent, ev_subj);
					if(he_result instanceof View)
						return (View)he_result;
				}
			}
			return convertView; // Return the completed view to render on screen
		}
	}*/
	public static class SlFragmentStatic extends androidx.fragment.app.Fragment {
		private int RcId;
		private int TabLayoutRcId;
		private int Ident; // Идентификатор фрагмента среди набора аналогичных.
		public int GetRcId() { return RcId; }
		public int GetIdent() { return Ident; }
		public SlFragmentStatic() // !Required empty public constructor
		{
		}
		//
	 	// Use this factory method to create a new instance of
	 	// this fragment using the provided parameters.
		//
		public static SlFragmentStatic newInstance(int ident, int rcId, int tabLayoutRcId)
		{
			SlFragmentStatic fragment = new SlFragmentStatic();
			Bundle args = new Bundle();
			args.putInt("IDENT", ident);
			args.putInt("RCID", rcId);
			if(tabLayoutRcId != 0)
				args.putInt("TABLAYOUTRCID", tabLayoutRcId);
			fragment.setArguments(args);
			return fragment;
		}
		public static SlFragmentStatic newInstance(int rcId, int tabLayoutRcId)
		{
			return newInstance(0, rcId, tabLayoutRcId);
		}
		@Override public void onCreate(Bundle savedInstanceState)
		{
			super.onCreate(savedInstanceState);
			Bundle b = getArguments();
			if(b != null) {
				RcId = b.getInt("RCID");
				TabLayoutRcId = b.getInt("TABLAYOUTRCID");
				Ident = b.getInt("IDENT");
			}
		}
		@Override public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
		{
			View result = inflater.inflate(RcId, container, false);
			if(result != null && result instanceof ViewGroup) {
				Context ctx = getContext();
				if(ctx != null) {
					if(ctx instanceof SlActivity) {
						((SlActivity)ctx).HandleEvent(EV_SETUPFRAGMENT, this, result);
						StyloQApp app_ctx = SlActivity.GetAppCtx(ctx);
						if(app_ctx != null)
							SubstituteStringSignatures((StyloQApp)app_ctx, (ViewGroup) result);
					}
					else if(ctx instanceof StyloQApp)
						SubstituteStringSignatures((StyloQApp) ctx, (ViewGroup) result);
				}
			}
			return result;
		}
		@Override public void onResume()
		{
			super.onResume();
			Context ctx = getActivity();
			if(ctx != null && ctx instanceof SlActivity)
				((SlActivity)ctx).HandleEvent(EV_SETVIEWDATA, getView(), null);
		}
		@Override public void onPause()
		{
			super.onPause();
			Context ctx = getActivity();
			if(ctx != null && ctx instanceof SlActivity)
				((SlActivity)ctx).HandleEvent(EV_GETVIEWDATA, getView(), null);
		}
		/*
		@Override
		public void onViewCreated(@NonNull View view, Bundle savedInstanceState)
		{
			Context ctx = view.getContext();
			if(ctx instanceof SlActivity) {
			}
			if(TabLayoutRcId != 0) {
				TabLayout lo_tab = view.findViewById(TabLayoutRcId);
				//new TabLayoutMediator(lo_tab, viewPager, (tab, position) -> tab.setText("OBJECT " + (position + 1))).attach();
			}
		}
		 */
	}
	public static boolean IsChildOf(View v, View supposedParentView)
	{
		boolean result = false;
		if(v != null && supposedParentView != null) {
			for(ViewParent p = v.getParent(); !result && p != null; p = p.getParent()) {
				if(p instanceof View && ((View)p) == supposedParentView)
					result = true;
			}
		}
		return result;
	}
	public static View FindViewById(Object viewContaiter, int ctlId)
	{
		View v = null;
		if(viewContaiter != null) {
			if(viewContaiter instanceof Activity)
				v = ((Activity)viewContaiter).findViewById(ctlId);
			if(viewContaiter instanceof SlFragmentStatic)
				v = ((SlFragmentStatic)viewContaiter).getView().findViewById(ctlId);
			else if(viewContaiter instanceof ViewGroup)
				v = ((ViewGroup) viewContaiter).findViewById(ctlId);
			else if(viewContaiter instanceof Dialog)
				v = ((Dialog) viewContaiter).findViewById(ctlId);
		}
		return v;
	}
	public static TextView FindTextViewById(Object viewContainer, int ctlId)
	{
		View v = FindViewById(viewContainer, ctlId);
		if(v != null) {
			if(v instanceof TextView)
				return (TextView)v;
			else if(v instanceof TextInputLayout) {
				ViewGroup vg = (ViewGroup)(TextInputLayout)v;
				do {
					ViewGroup inner_vg = null;
					for(int i = 0; i < vg.getChildCount(); i++) {
						View inner_v = vg.getChildAt(i);
						if(inner_v != null) {
							if(inner_v instanceof TextView)
								return (TextView)inner_v;
							else if(inner_v instanceof ViewGroup) {
								inner_vg = (ViewGroup)inner_v;
								break;
							}
						}
					}
					vg = inner_vg;
				} while(vg != null);
			}
		}
		return null;
	}
	public static EditText FindEditTextById(Object viewContainer, int ctlId)
	{
		View v = FindViewById(viewContainer, ctlId);
		if(v != null) {
			if(v instanceof EditText)
				return (EditText)v;
			else if(v instanceof TextInputLayout) {
				ViewGroup vg = (ViewGroup)(TextInputLayout)v;
				do {
					ViewGroup inner_vg = null;
					for(int i = 0; i < vg.getChildCount(); i++) {
						View inner_v = vg.getChildAt(i);
						if(inner_v != null) {
							if(inner_v instanceof EditText)
								return (EditText)inner_v;
							else if(inner_v instanceof ViewGroup) {
								inner_vg = (ViewGroup)inner_v;
								break;
							}
						}
					}
					vg = inner_vg;
				} while(vg != null);
			}
		}
		return null;
	}
	public static CheckBox FindCheckboxById(Object viewContainer, int ctlId)
	{
		View v = FindViewById(viewContainer, ctlId);
		return (v != null && v instanceof CheckBox) ? (CheckBox)v : null;
	}
	public static void SetCtrlVisibility(Object viewContainer, int ctlId, int visiMode)
	{
		View v = FindViewById(viewContainer, ctlId);
		if(v != null)
			v.setVisibility(visiMode);
	}
	public static void SetCtrlVisibility(View view, int visiMode)
	{
		if(view != null)
			view.setVisibility(visiMode);
	}
	public static boolean SetCtrlString(Object viewContainer, int ctlId, String text)
	{
		boolean ok = false;
		EditText ctl = FindEditTextById(viewContainer, ctlId);
		if(ctl != null) {
			ctl.setText(text);
			ok = true;
		}
		else {
			TextView tv = FindTextViewById(viewContainer, ctlId);
			if(tv != null) {
				tv.setText(text);
				ok = true;
			}
		}
		return ok;
	}
	public static String GetCtrlString(Object viewContainer, int ctlId)
	{
		EditText ctl = FindEditTextById(viewContainer, ctlId);
		return (ctl != null) ? ctl.getText().toString() : null;
	}
	public static void SetCheckboxState(Object viewContainer, int ctlId, boolean checked)
	{
		CheckBox cb = FindCheckboxById(viewContainer, ctlId);
		if(cb != null)
			cb.setChecked(checked);
	}
	public static boolean GetCheckboxState(Object viewContainer, int ctlId)
	{
		CheckBox cb = FindCheckboxById(viewContainer, ctlId);
		return (cb != null) ? cb.isChecked() : false;
	}
	public static void SetupTabLayoutStyle(TabLayout loTab)
	{
		if(loTab != null) {
			ViewGroup tabs = (ViewGroup)loTab.getChildAt(0);
			for(int tabidx = 0; tabidx < tabs.getChildCount() - 1; tabidx++) {
				View tab = tabs.getChildAt(tabidx);
				LinearLayout.LayoutParams layout_params = (LinearLayout.LayoutParams)tab.getLayoutParams();
				layout_params.setMarginStart(4);
				layout_params.setMarginEnd(4);
				tab.setLayoutParams(layout_params);
				loTab.requestLayout();
			}
		}
	}
	public static void SetupTabLayoutListener(EventHandler eh, TabLayout loTab, ViewPager2 viewPager)
	{
		if(loTab != null && viewPager != null) {
			loTab.addOnTabSelectedListener(new TabLayout.OnTabSelectedListener() {
				@Override public void onTabSelected(TabLayout.Tab tab)
				{
					int pos = tab.getPosition();
					viewPager.setCurrentItem(pos, false);
				}
				@Override public void onTabUnselected(TabLayout.Tab tab)
				{
				}
				@Override public void onTabReselected(TabLayout.Tab tab)
				{
				}
			});
			//
			viewPager.registerOnPageChangeCallback(new ViewPager2.OnPageChangeCallback() {
				@Override public void onPageSelected(int position)
				{
					loTab.selectTab(loTab.getTabAt(position));
					if(eh != null) {
						eh.HandleEvent(EV_TABSELECTED, viewPager, new Integer(position));
					}
				}
			});
		}
	}
	public static abstract class SlActivity extends AppCompatActivity implements EventHandler {
		private ActivityResultLauncher <Intent> StartForResult;
		private Timer RTmr;
		public static StyloQApp GetAppCtx(Context ctx)
		{
			return (ctx != null && ctx instanceof SlActivity) ? ((SlActivity)ctx).GetAppCtx() : null;
		}
		public StyloQApp GetAppCtx()
		{
			Context ctx = getApplication();
			return (ctx != null && ctx instanceof StyloQApp) ? (StyloQApp)ctx : null;
		}
		//
		// Descr: Функция запуска другой SlActivity с целью получения результата исполнения //
		//
		public void LaunchOtherActivity(Intent intent)
		{
			if(StartForResult != null)
				StartForResult.launch(intent);
		}
		@Override protected void onCreate(Bundle savedInstanceState)
		{
			super.onCreate(savedInstanceState);
			{
				SlActivity _this_activity = this;
				StartForResult = registerForActivityResult(new ActivityResultContracts.StartActivityForResult(),
						new ActivityResultCallback <ActivityResult>() {
							@Override public void onActivityResult(ActivityResult result)
							{
								_this_activity.HandleEvent(EV_ACTIVITYRESULT, null, result);
							}
						});
			}
			HandleEvent(EV_CREATE, this, savedInstanceState);
		}
		@Override protected void onStart()
		{
			super.onStart();
			HandleEvent(EV_ACTIVITYSTART, this, null);
		}
		@Override public void onResume()
		{
			super.onResume();
			HandleEvent(EV_ACTIVITYRESUME, this, null);
		}
		@Override protected void onDestroy()
		{
			super.onDestroy();
			HandleEvent(EV_DESTROY, this, null);
			if(RTmr != null) {
				RTmr.cancel();
				RTmr = null;
			}
		}
		protected void ScheduleRTmr(java.util.TimerTask task, long delay, long period)
		{
			if(task != null) {
				RTmr = new Timer();
				RTmr.schedule(task, delay/*1000*/, period/*750*/);
			}
			else if(RTmr != null) {
				RTmr.cancel();
				RTmr = null;
			}
		}
		public void OnButtonClk(View view) { HandleEvent(EV_COMMAND, view, null); }
		public void SetupRecyclerListView(View parentView, int rcListView, int rcItemView)
		{
			View view = (parentView == null) ? findViewById(rcListView) : parentView.findViewById(rcListView);
			if(view != null && view instanceof RecyclerView) {
				RecyclerListAdapter adapter = new RecyclerListAdapter(this, null, rcListView, rcItemView);
				((RecyclerView)view).setAdapter(adapter);
				//view.addItemDecoration(new DividerItemDecoration(this, DividerItemDecoration.VERTICAL));
			}
		}
		public void SetupRecyclerListView(View parentView, int rcListView, LinearLayout itemLayout)
		{
			View view = (parentView == null) ? findViewById(rcListView) : parentView.findViewById(rcListView);
			if(view != null && view instanceof RecyclerView) {
				RecyclerListAdapter adapter = new RecyclerListAdapter(this, null, rcListView, itemLayout);
				((RecyclerView)view).setAdapter(adapter);
				//view.addItemDecoration(new DividerItemDecoration(this, DividerItemDecoration.VERTICAL));
			}
		}
		public void SetRecyclerListFocusedIndex(RecyclerView.Adapter adapter, int idx)
		{
			if(adapter != null && adapter instanceof RecyclerListAdapter)
				((RecyclerListAdapter)adapter).SetFocusedIndex(idx);
		}
		public void SetRecyclerListFocusedIndex(View parentView, int rcListView, int idx)
		{
			View view = (parentView == null) ? findViewById(rcListView) : parentView.findViewById(rcListView);
			if(view != null && view instanceof RecyclerView)
				SetRecyclerListFocusedIndex(((RecyclerView)view).getAdapter(), idx);
		}
		/*public int GetRecyclerListFocusedIndex(RecyclerView.Adapter adapter)
		{
			return (adapter != null && adapter instanceof RecyclerListAdapter) ? ((RecyclerListAdapter)adapter).GetFocusedIndex() : -1;
		}*/
		public int GetListFocusedIndex(Object adapter)
		{
			int    result = -1;
			if(adapter != null) {
				if(adapter instanceof RecyclerListAdapter)
					result = ((RecyclerListAdapter)adapter).GetFocusedIndex();
				else if(adapter instanceof InternalArrayAdapter)
					result = ((InternalArrayAdapter)adapter).GetFocusedIndex();
			}
			return result;
		}
		public int GetRecyclerListFocusedIndex(View parentView, int rcListView)
		{
			View view = (parentView == null) ? findViewById(rcListView) : parentView.findViewById(rcListView);
			return (view != null && view instanceof RecyclerView) ? GetListFocusedIndex(((RecyclerView)view).getAdapter()) : -1;
		}
		public void SetupViewPagerWithFragmentAdapter(int rcViewPager)
		{
			View view = findViewById(rcViewPager);
			if(view != null && view instanceof ViewPager2) {
				((ViewPager2)view).setUserInputEnabled(false); // Этот вызов предотвращает переключение между табуляторами скользящим
					// движением по экрану. Будучи весьма эффектной, эта функция мешает управлять конкретными страницами,
					// которые сами по себе бывают довольно сложными.
				FragmentAdapter adapter = new FragmentAdapter(this);
				((ViewPager2)view).setAdapter(adapter);
			}
		}
		public void SetupListView(int rcListView, int rcItemView, ArrayList data)
		{
			ListView list_view = findViewById(rcListView);
			if(list_view != null) {
				InternalArrayAdapter adapter = new InternalArrayAdapter(this, rcItemView, data);
				list_view.setAdapter(adapter);
				adapter.setNotifyOnChange(true);
				list_view.setOnItemClickListener(new AdapterView.OnItemClickListener() {
					@Override public void onItemClick(AdapterView<?> parent, View view, int position, long id)
					{
						Object item = (Object)parent.getItemAtPosition(position);
						Context ctx = parent.getContext();
						if(item != null && ctx != null && ctx instanceof SlActivity) {
							SlActivity activity = (SlActivity)parent.getContext();
							ListViewEvent ev_subj = new ListViewEvent();
							ev_subj.ItemIdx = position;
							ev_subj.ItemId = id;
							ev_subj.ItemObj = item;
							ev_subj.ItemView = view;
							//ev_subj.ParentView = parent;
							activity.HandleEvent(EV_LISTVIEWITEMCLK, parent, ev_subj);
						}
					}
				});
				list_view.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
					@Override public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id)
					{
						boolean result = false;
						Object item = (Object)parent.getItemAtPosition(position);
						Context ctx = parent.getContext();
						if(item != null && ctx != null && ctx instanceof SlActivity) {
							SlActivity activity = (SlActivity)parent.getContext();
							ListViewEvent ev_subj = new ListViewEvent();
							ev_subj.ItemIdx = position;
							ev_subj.ItemId = id;
							ev_subj.ItemObj = item;
							ev_subj.ItemView = view;
							//ev_subj.ParentView = parent;
							Object he_result = activity.HandleEvent(EV_LISTVIEWITEMLONGCLK, parent, ev_subj);
							if(he_result != null && he_result instanceof Boolean)
								result = (Boolean)he_result;
						}
						return result;
					}
				});
			}
		}
		/*private EditText FindEditTextById(int ctlId)
		{
			View v = findViewById(ctlId);
			return (v != null && v instanceof EditText) ? (EditText)v : null;
		}
		public boolean SetCtrlString(int ctlId, String text)
		{
			//return SLib.SetCtrlString((ViewGroup)this, ctlId, text);
			boolean ok = false;
			View v = findViewById(ctlId);
			if(v != null) {
				if(v instanceof TextView) {
					((TextView)v).setText(text);
					ok = true;
				}
				else if(v instanceof EditText) {
					((EditText)v).setText(text);
					ok = true;
				}
			}
			return ok;
		}*/
		public void NotifyListView_DataSetChanged(int rcListView)
		{
			ListView list_view = findViewById(rcListView);
			if(list_view != null) {
				InternalArrayAdapter adapter = (InternalArrayAdapter)list_view.getAdapter();
				if(adapter != null)
					adapter.notifyDataSetChanged();
			}
		}
	}
	public static void RequestRecyclerListViewPosition(RecyclerView view, int idx)
	{
		if(view != null && idx >= 0) {
			view.postDelayed(new Runnable() { @Override public void run() { view.scrollToPosition(idx); } }, 500);
		}
	}
	static class StrAssocSpinnerAdapter implements SpinnerAdapter {
		StrAssocArray Data;
		Context Ctx;
		public StrAssocSpinnerAdapter(Context ctx, StrAssocArray data)
		{
			Ctx = ctx;
			Data = data;
		}
		// Descr: Возвращает идентификатор элемента по заданной позиции idx [0..]
		int GetIdByIdx(int idx) { return (Data != null && idx >= 0 && idx < Data.GetCount()) ? Data.GetByPos(idx).Id : 0; }
		// Descr: Возвращает позицию элемента [0..] по заданному идентификатору id
		// Если идентификатор есть в списке, то возвращает позицию [0..]
		// Если идентификатор не найден, то возвращает -1.
		int GetIdxById(int id)
		{
			return (Data != null) ? Data.Search(id) : -1;
		}
		@NonNull String GetTextByIdx(int idx)
		{
			String result = null;
			if(Data != null && idx >= 0 && idx < Data.GetCount()) {
				StrAssocArray.Item item = Data.GetByPos(idx);
				result = (item != null && item.Txt != null) ? item.Txt : "";
			}
			else
				result = "";
			return result;
		}
		@Override public void registerDataSetObserver(DataSetObserver observer)
		{
		}
		@Override public void unregisterDataSetObserver(DataSetObserver observer)
		{
		}
		@Override public int getCount()
		{
			return (Data != null) ? Data.GetCount() : 0;
		}
		@Override public Object getItem(int idx) { return (Data != null && idx < Data.GetCount()) ? Data.GetTextByPos(idx) : ""; }
		@Override public long getItemId(int idx)
		{
			if(Data != null && idx < Data.GetCount()) {
				StrAssocArray.Item item = Data.GetByPos(idx);
				return (item != null) ? item.Id : 0;
			}
			else
				return 0;
		}
		@Override public boolean hasStableIds()
		{
			return true;
		}
		@Override public View getView(int idx, View convertView, ViewGroup parent)
		{
			TextView view = null;
			if(convertView != null && convertView instanceof TextView) {
				view = (TextView)convertView;
			}
			else {
				view = new TextView(Ctx);
				view.setPadding(8, 0, 0, 0);
			}
			String t = GetTextByIdx(idx);
			view.setText(t);
			return view;
		}
		@Override public View getDropDownView(int idx, View convertView, ViewGroup parent)
		{
			return this.getView(idx, convertView, parent);
		}
		@Override public int getItemViewType(int position)
		{
			return IGNORE_ITEM_VIEW_TYPE;
		}
		@Override public int getViewTypeCount()
		{
			return 1; // must be 1!
		}
		@Override public boolean isEmpty()
		{
			return (Data == null || Data.GetCount() == 0);
		}
	}
	private static Spinner FindSpinner(Object parentView, int rcId)
	{
		View v = null;
		if(parentView != null) {
			if(parentView instanceof ViewGroup)
				v = ((ViewGroup)parentView).findViewById(rcId);
			else if(parentView instanceof Dialog)
				v = ((Dialog)parentView).findViewById(rcId);
		}
		return (v != null && v instanceof Spinner) ? (Spinner)v : null;
	}
	public static void SetupStrAssocCombo(StyloQApp ctx, Object parentView, int rcId, StrAssocArray data, int initValue)
	{
		Spinner spinner_view = FindSpinner(parentView, rcId);
		if(spinner_view != null) {
			StrAssocSpinnerAdapter adapter = new StrAssocSpinnerAdapter(ctx, data);
			spinner_view.setAdapter(adapter);
			int sel_pos = (initValue != 0) ? adapter.GetIdxById(initValue) : AdapterView.INVALID_POSITION;
			spinner_view.setSelection(sel_pos);
			{
				spinner_view.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
					public void onItemSelected(AdapterView<?> adapterView, View view, int idx, long id)
					{
						Object item = view;
						Context ctx = adapterView.getContext();
						if(item != null && ctx != null && ctx instanceof SLib.SlActivity) {
							SLib.SlActivity activity = (SLib.SlActivity)ctx;
							SLib.ListViewEvent ev_subj = new SLib.ListViewEvent();
							ev_subj.ItemIdx = idx;
							ev_subj.ItemId = id;
							ev_subj.ItemObj = item;
							ev_subj.ItemView = view;
							//ev_subj.ParentView = adapterView;
							activity.HandleEvent(SLib.EV_CBSELECTED, adapterView, ev_subj);
						}
					}
					public void onNothingSelected(AdapterView<?> adapterView)
					{
						return;
					}
				});
			}
		}
	}
	public static boolean SetStrAssocComboData(Object parentView, int rcId, int value)
	{
		boolean ok = false;
		Spinner spinner_view = FindSpinner(parentView, rcId);
		if(spinner_view != null) {
			SpinnerAdapter a = spinner_view.getAdapter();
			if(a != null && a instanceof StrAssocSpinnerAdapter) {
				final int idx = ((StrAssocSpinnerAdapter)a).GetIdxById(value);
				if(idx >= 0) {
					spinner_view.setSelection(idx);
					ok = true;
				}
			}
		}
		return  ok;
	}
	public static int GetStrAssocComboData(Object parentView, int rcId)
	{
		int    result = 0;
		Spinner spinner_view = FindSpinner(parentView, rcId);
		if(spinner_view != null) {
			final int idx = spinner_view.getSelectedItemPosition();
			if(idx != AdapterView.INVALID_POSITION) {
				SpinnerAdapter a = spinner_view.getAdapter();
				if(a != null && a instanceof StrAssocSpinnerAdapter)
					result = ((StrAssocSpinnerAdapter)a).GetIdByIdx(idx);
			}
		}
		return result;
	}
	public static int CalcListViewHeight(ListView lv)
	{
		int result = 0;
		if(lv != null) {
			Adapter adapter = lv.getAdapter();
			if(adapter != null) {
				final int _count_a = adapter.getCount();
				//assert(_count_a == detail_list.size());
				if(_count_a > 0) {
					for(int aidx = 0; aidx < _count_a; aidx++) {
						View local_list_item_view = adapter.getView(aidx, null, lv);
						if(local_list_item_view != null) {
							local_list_item_view.measure(View.MeasureSpec.UNSPECIFIED, View.MeasureSpec.UNSPECIFIED);
							int _ih1 = local_list_item_view.getMeasuredHeight();
							result += _ih1;
						}
					}
					int total_dividers_height = 0;// lv.getDividerHeight() * (_count_a - 1);
					result += total_dividers_height;
					//ViewGroup.LayoutParams params = listView.getLayoutParams();
					//params.height = total_items_height + total_dividers_height;
					//listView.setLayoutParams(params);
					//listView.requestLayout();
				}
			}
		}
		return result;
	}
	public static void SetupImage(Activity activity, View imgView, String blobSignature, boolean dontRemoveIfNoImg)
	{
		if(imgView != null && imgView instanceof ImageView) {
			ImageView imgv = (ImageView)imgView;
			if(SLib.GetLen(blobSignature) > 0) {
				imgv.setVisibility(View.VISIBLE);
				Glide.with(activity).load(GlideSupport.ModelPrefix + blobSignature)./*centerCrop().*/
					/*signature(new ObjectKey(blobSignature)).*/
					apply(RequestOptions.bitmapTransform(new RoundedCorners(16/*R.dimen.outerimgroundedcorners*/))).into(imgv);
			}
			else if(!dontRemoveIfNoImg)
				imgv.setVisibility(View.GONE);
		}
	}
	public static String GetObjectTitle(StyloQApp ctx, int objType)
	{
		return ctx._StrStor.GetString(ctx.GetCurrentLang(), ppstr2.PPSTR_OBJNAMES, objType);
	}
	public static String ExpandString(StyloQApp ctx, String signature)
	{
		return ctx._StrStor.ExpandString(ctx.GetCurrentLang(), signature);
	}
	public static boolean SubstituteStringSignatures(StyloQApp ctx, ViewGroup view)
	{
		boolean result = false;
		if(ctx != null && view != null) {
			for(int i = 0; i < view.getChildCount(); i++) {
				View v = view.getChildAt(i);
				if(v instanceof Button) {
					Button btn = (Button)v;
					String text = btn.getText().toString();
					String exp_text = ctx._StrStor.ExpandString(ctx.GetCurrentLang(), text);
					if(!exp_text.equals(text))
						btn.setText(exp_text);
				}
				else if(v instanceof TextInputLayout) {
					TextInputLayout et = (TextInputLayout)v;
					CharSequence cs_text = et.getHint();
					if(GetLen(cs_text) > 0) {
						String text = cs_text.toString();
						String exp_text = ctx._StrStor.ExpandString(ctx.GetCurrentLang(), text);
						if(!exp_text.equals(text))
							et.setHint(exp_text);
					}
				}
				else if(v instanceof EditText) {
					EditText et = (EditText)v;
					CharSequence cs_text = et.getHint();
					if(GetLen(cs_text) > 0) {
						String text = cs_text.toString();
						String exp_text = ctx._StrStor.ExpandString(ctx.GetCurrentLang(), text);
						if(!exp_text.equals(text))
							et.setHint(exp_text);
					}
				}
				else if(v instanceof TextView) {
					TextView tv = (TextView)v;
					CharSequence cs_text = tv.getText();
					if(GetLen(cs_text) > 0) {
						String text = cs_text.toString();
						String exp_text = ctx._StrStor.ExpandString(ctx.GetCurrentLang(), text);
						if(!exp_text.equals(text))
							tv.setText(exp_text);
					}
				}
				else if(v instanceof ViewGroup) {
					boolean local_result = SubstituteStringSignatures(ctx, (ViewGroup) v); // @recursion
					if(local_result)
						result = local_result;
				}
			}
		}
		return result;
	}
	public static abstract class SlDialog extends Dialog implements EventHandler {
		protected Object Data;
		protected int Ident; // id, установленный у лайаута вернхнего уровня.
		SlDialog(Context ctx, int ident, Object data)
		{
			super(ctx);
			Ident = ident;
			Data = (data != null) ? data : null;
		}
		private void SetupButtonHandlerLoop(ViewGroup view)
		{
			for(int i = 0; i < view.getChildCount(); i++) {
				View v = view.getChildAt(i);
				if(v instanceof Button) {
					if(v.getId() != 0) {
						Button btn = (Button)v;
						btn.setOnClickListener(new View.OnClickListener()
							{ @Override public void onClick(View v) { HandleEvent(EV_COMMAND, v, null); }});
					}
				}
				else if(v instanceof ImageButton) {
					if(v.getId() != 0) {
						ImageButton btn = (ImageButton)v;
						btn.setOnClickListener(new View.OnClickListener()
							{ @Override public void onClick(View v) { HandleEvent(EV_COMMAND, v, null); }});
					}
				}
				else if(v instanceof ImageView) {
					if(v.getId() != 0) {
						ImageView btn = (ImageView)v;
						btn.setOnClickListener(new View.OnClickListener()
							{ @Override public void onClick(View v) { HandleEvent(EV_COMMAND, v, null); }});
					}
				}
				else if(v instanceof ViewGroup) {
					this.SetupButtonHandlerLoop((ViewGroup)v); // @recursion
				}
			}
		}
		@Override public void onCreate(Bundle savedInstanceState)
		{
			super.onCreate(savedInstanceState);
			HandleEvent(EV_CREATE, this, savedInstanceState);
			if(Ident > 0) {
				View tv = findViewById(Ident);
				if(tv != null && tv instanceof ViewGroup) {
					ViewGroup vg = (ViewGroup)tv;
					SetupButtonHandlerLoop(vg);
					Context ctx = getContext();
					if(ctx != null) {
						StyloQApp app_ctx = (StyloQApp)ctx.getApplicationContext();
						if(app_ctx != null)
							SubstituteStringSignatures(app_ctx, vg);
					}
				}
			}
		}
	}
	enum ConfirmationResult {
		YES,
		NO,
		CANCEL
	}
	public interface ConfirmationListener {
		void OnResult(ConfirmationResult r);
	}
	public static class ConfirmDialog extends SlDialog {
		private int LayoutIdent;
		private ConfirmationListener OnResult;
		ConfirmDialog(Context ctx, int ident, int layoutIdent, String text, ConfirmationListener result)
		{
			super(ctx, ident, text);
			LayoutIdent = layoutIdent;
			OnResult = result;
		}
		@Override public Object HandleEvent(int ev, Object srcObj, Object subj)
		{
			Object result = null;
			switch(ev) {
				case SLib.EV_CREATE:
				{
					//requestWindowFeature(Window.FEATURE_NO_TITLE);
					setContentView(/*R.layout.dialog_personevent*/LayoutIdent);
					StyloQApp app_ctx = SLib.SlActivity.GetAppCtx(getContext());
					if(app_ctx != null) {
						setTitle(SLib.ExpandString(app_ctx, "@{confirm}"));
					}
					if(Data != null && Data instanceof String) {
						SetCtrlString(this, R.id.CTL_CONFIRM_QUEST, (String)Data);
					}
				}
				break;
				case SLib.EV_COMMAND:
					if(srcObj != null && srcObj instanceof View) {
						final int view_id = ((View)srcObj).getId();
						if(view_id == R.id.CTL_BUTTON_YES) {
							StyloQApp app_ctx = SLib.SlActivity.GetAppCtx(getContext());
							if(OnResult != null) {
								OnResult.OnResult(ConfirmationResult.YES);
							}
							/*
							if(app_ctx != null)
								app_ctx.HandleEvent(SLib.EV_IADATAEDITCOMMIT, this, "yes");
							 */
							this.dismiss();
						}
						else if(view_id == R.id.CTL_BUTTON_NO) {
							if(OnResult != null) {
								OnResult.OnResult(ConfirmationResult.NO);
							}
							this.dismiss();
						}
					}
					break;
			}
			return null;
		}
	}
	public static void Confirm_YesNo(Context ctx, String text, ConfirmationListener result)
	{
		ConfirmDialog dlg = new ConfirmDialog(ctx, R.id.DLG_CONFIRM_YESNO, R.layout.dialog_confirm_yesno, text, result);
		dlg.show();
	}
}
