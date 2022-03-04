// SFILE.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
//
#include <slib-internal.h>
#pragma hdrstop
#include <share.h>
#include <sys\locking.h>

#if 0 // @construction {
/*static*/void * SIo::SCreateFile(int ioType, const char * pName, uint32 mode, uint32 secMode /*= 0*/)
{
	SString name = pName;
	if(ioType) {
		switch(ioType) {
			case tPipe:
				name = "\\\\.\\pipe\\";
				break;
			case tSerial:
				name = "\\\\.\\pipe\\";
				break;
		}
	}
    return ::CreateFile()
}
#endif // } 0 @construction

/* mimetypes.dict
binary: application/binary
binary: application/binary-data
binary: application/octet-stream
binary: application/x-binary
binary: binary/octet-stream
binary: application/vnd.google.safebrowsing-chunk
binary: application/vnd.google.safebrowsing-update
binary: application/ocsp-request
binary: application/ocsp-response
binary:exe application/x-msdos-program
binary:msi application/x-msi
binary:msp application/microsoftpatch
archive:7z application/x-7z-compressed
archive:bz2 application/x-bzip
archive:bz2 application/x-bzip2
archive:cab application/vnd.ms-cab-compressed
archive:cab application/x-cab
archive:deb application/x-deb
archive:deb application/x-debian-package
archive:gz  application/gzip
archive:gz  application/x-gzip
archive:gz  application/x-gzip-compressed
archive:gz  application/x-tar-gz
archive:rar application/rar
archive:rar application/winrar
archive:rar application/x-rar
archive:rar application/x-rar-compressed
archive:rpm application/x-redhat-package-manager
archive:rpm application/x-rpm
archive:swf application/x-shockwave-flash
archive:swf image/swf
archive:tar application/x-tar
archive:zip application/x-zip
archive:zip application/x-zip-compressed
archive:zip application/zip
archive:kmz application/vnd.google-earth.kmz
archive:jar application/java-archive
archive:jar application/x-java-archive
archive:wmz application/x-ms-wmz
text: application/x-www-form-urlencoded
text:css text/css
text:csv text/csv
text:html application/html
text:html text/html
text:js application/javascript
text:js application/javascript,application/x-javascript
text:js application/javascript,application/x-javascript,text/javascript
text:js application/js
text:js application/x-javascript
text:js text/javascript
text:js text/x-javascript
text:js text/x-js
text:json application/json
text:json application/x-json
text:json json/text
text:json text/json
text:json text/x-json
text:kml application/vnd.google-earth.kml+xml
text:rss application/rss+xml
text:smil application/smil
text:torrent application/x-bittorrent
text:txt plain/text
text:txt text/plain
text:txt text/plaintext
text:txt text/txt
text:xml application/xml
text:xml text/xml
text:xml txt/xml
text:wml text/vnd.wap.wml
video:f4f video/f4f
video:f4f video/x-f4f
video:flv flv-application/octet-stream
video:flv video/x-flv
video:3gp video/3gpp
video:asf video/x-ms-asf
video:avi video/avi
video:m4v video/m4v
video:m4v video/x-m4v
video:mov video/mov
video:mov video/quicktime
video:mp4 video/mp4
video:mpeg video/mpeg
video:ogg video/ogg
video:webm video/webm
video:wmv video/wmv
video:wmv video/x-ms-wmv
audio:m4a audio/x-m4a
audio:amr audio/amr
audio:mid audio/midi
audio:mp3 audio/mp3
audio:mp3 audio/mpeg
audio:mp3 audio/mpeg3
audio:mp4 audio/mp4
audio:wav audio/wav
audio:wav audio/x-wav
audio:ogg application/ogg
audio:ogg audio/ogg
audio:webm audio/webm
document:crt application/x-x509-ca-cert
document:doc application/haansoftdoc
document:doc application/msword
document:doc application/vnd.ms-word
document:doc application/x-msword
document:doc application/x-unknown-application-msword
document:docm application/vnd.ms-word.document.macroenabled.12
document:docx application/haansoftdocx
document:docx application/vnd.ms-word.document.12
document:docx application/vnd.openxmlformats-officedocument.wordprocessingml
document:docx application/vnd.openxmlformats-officedocument.wordprocessingml.d
document:docx application/vnd.openxmlformats-officedocument.wordprocessingml.document
document:dotx application/vnd.openxmlformats-officedocument.wordprocessingml.template
document:eml message/rfc822
document:eml message/rfc822-headers
document:eml text/rfc822-header
document:eml text/rfc822-headers
document:mdb application/msaccess
document:mdb application/x-msaccess
document:mpp application/vnd.ms-project
document:odc application/vnd.oasis.opendocument.chart
document:odf application/vnd.oasis.opendocument.formula
document:odg application/vnd.oasis.opendocument.graphics
document:odi application/vnd.oasis.opendocument.image
document:odm application/vnd.oasis.opendocument.text-master
document:odp application/vnd.oasis.opendocument.presentation
document:ods application/vnd.oasis.opendocument.spreadsheet
document:odt application/vnd.oasis.opendocument.text
document:otc application/vnd.oasis.opendocument.chart-template
document:otf application/vnd.oasis.opendocument.formula-template
document:otg application/vnd.oasis.opendocument.graphics-template
document:oth application/vnd.oasis.opendocument.text-web
document:oti application/vnd.oasis.opendocument.image-template
document:otp application/vnd.oasis.opendocument.presentation-template
document:ots application/vnd.oasis.opendocument.spreadsheet-template
document:ott application/vnd.oasis.opendocument.text-template
document:pdf application/pdf
document:pdf application/vnd.pdf
document:pdf application/x-pdf
document:pdf file/pdf
document:plist application/x-apple-plist
document:ppt application/ms-powerpoint
document:ppt application/mspowerpoint
document:ppt application/vnd.ms-powerpoint
document:pptx application/vnd.ms-powerpoint.12
document:pptx application/vnd.openxmlformats-officedocument.presentationml
document:pptx application/vnd.openxmlformats-officedocument.presentationml.presentation
document:rtf application/rtf
document:rtf text/richtext
document:rtf text/rtf
document:thmx application/vnd.ms-officetheme
document:wbxml application/vnd.ms-sync.wbxml
document:xls application/excel
document:xls application/haansoftxls
document:xls application/ms-excel
document:xls application/msexcel
document:xls application/vnd.ms-excel
document:xls application/x-excel
document:xls application/x-msexcel
document:xlsb application/vnd.ms-excel.sheet.binary.macroenabled.12
document:xlsm application/vnd.ms-excel.sheet.macroenabled.12
document:xlsx application/vnd.ms-excel.12
document:xlsx application/vnd.openxmlformats-officedocument.spreadsheetml.sh
document:xlsx application/vnd.openxmlformats-officedocument.spreadsheetml.sheet
document:xlsx application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml
image:bmp image/bmp
image:bmp image/vnd.wap.bmp
image:bmp image/x-ms-bmp
image:djvu image/vnd.djvu
image:dwg image/vnd.dwg
image:gif application/gif
image:gif image/gif
image:ico image/ico
image:ico image/icons
image:ico image/vnd.microsoft.icon
image:ico image/x-ico
image:ico image/x-icon
image:jpg image/jpeg
image:jpg image/jpg
image:jpg image/pjpeg
image:jpg image/x-citrix-jpeg
image:mdi image/vnd.ms-modi
image:wbmp image/vnd.wap.wbmp
image:png image/png
image:png image/x-png
image:svg image/svg+xml
image:tif image/tif
image:tiff image/tiff
image:tiff image/x-tiff
image:webp image/webp
font:eot application/vnd.ms-fontobject
font:eot application/x-font-eot
font:eot font/eot
font:otf application/x-font-otf
font:ttf application/x-font-ttf
font:ttf font/truetype
font:ttf font/ttf
font:woff application/font-woff
font:woff application/x-font-woff
font:woff application/x-woff
font:woff font/woff
font:woff font/x-woff
application:wps application/vnd.ms-works
//
//
//
{".323", "text/h323"},
{".3g2", "video/3gpp2"},
{".3gp", "video/3gpp"},
{".3gp2", "video/3gpp2"},
{".3gpp", "video/3gpp"},
{".7z", "application/x-7z-compressed"},
{".aa", "audio/audible"},
{".AAC", "audio/aac"},
{".aaf", "application/octet-stream"},
{".aax", "audio/vnd.audible.aax"},
{".ac3", "audio/ac3"},
{".aca", "application/octet-stream"},
{".accda", "application/msaccess.addin"},
{".accdb", "application/msaccess"},
{".accdc", "application/msaccess.cab"},
{".accde", "application/msaccess"},
{".accdr", "application/msaccess.runtime"},
{".accdt", "application/msaccess"},
{".accdw", "application/msaccess.webapplication"},
{".accft", "application/msaccess.ftemplate"},
{".acx", "application/internet-property-stream"},
{".AddIn", "text/xml"},
{".ade", "application/msaccess"},
{".adobebridge", "application/x-bridge-url"},
{".adp", "application/msaccess"},
{".ADT", "audio/vnd.dlna.adts"},
{".ADTS", "audio/aac"},
{".afm", "application/octet-stream"},
{".ai", "application/postscript"},
{".aif", "audio/aiff"},
{".aifc", "audio/aiff"},
{".aiff", "audio/aiff"},
{".air", "application/vnd.adobe.air-application-installer-package+zip"},
{".amc", "application/mpeg"},
{".anx", "application/annodex"},
{".apk", "application/vnd.android.package-archive" },
{".application", "application/x-ms-application"},
{".art", "image/x-jg"},
{".asa", "application/xml"},
{".asax", "application/xml"},
{".ascx", "application/xml"},
{".asd", "application/octet-stream"},
{".asf", "video/x-ms-asf"},
{".ashx", "application/xml"},
{".asi", "application/octet-stream"},
{".asm", "text/plain"},
{".asmx", "application/xml"},
{".aspx", "application/xml"},
{".asr", "video/x-ms-asf"},
{".asx", "video/x-ms-asf"},
{".atom", "application/atom+xml"},
{".au", "audio/basic"},
{".avi", "video/x-msvideo"},
{".axa", "audio/annodex"},
{".axs", "application/olescript"},
{".axv", "video/annodex"},
{".bas", "text/plain"},
{".bcpio", "application/x-bcpio"},
{".bin", "application/octet-stream"},
{".bmp", "image/bmp"},
{".c", "text/plain"},
{".cab", "application/octet-stream"},
{".caf", "audio/x-caf"},
{".calx", "application/vnd.ms-office.calx"},
{".cat", "application/vnd.ms-pki.seccat"},
{".cc", "text/plain"},
{".cd", "text/plain"},
{".cdda", "audio/aiff"},
{".cdf", "application/x-cdf"},
{".cer", "application/x-x509-ca-cert"},
{".cfg", "text/plain"},
{".chm", "application/octet-stream"},
{".class", "application/x-java-applet"},
{".clp", "application/x-msclip"},
{".cmd", "text/plain"},
{".cmx", "image/x-cmx"},
{".cnf", "text/plain"},
{".cod", "image/cis-cod"},
{".config", "application/xml"},
{".contact", "text/x-ms-contact"},
{".coverage", "application/xml"},
{".cpio", "application/x-cpio"},
{".cpp", "text/plain"},
{".crd", "application/x-mscardfile"},
{".crl", "application/pkix-crl"},
{".crt", "application/x-x509-ca-cert"},
{".cs", "text/plain"},
{".csdproj", "text/plain"},
{".csh", "application/x-csh"},
{".csproj", "text/plain"},
{".css", "text/css"},
{".csv", "text/csv"},
-- {".cur", "application/octet-stream"},
{".cxx", "text/plain"},
{".dat", "application/octet-stream"},
{".datasource", "application/xml"},
{".dbproj", "text/plain"},
{".dcr", "application/x-director"},
{".def", "text/plain"},
{".deploy", "application/octet-stream"},
{".der", "application/x-x509-ca-cert"},
{".dgml", "application/xml"},
{".dib", "image/bmp"},
{".dif", "video/x-dv"},
{".dir", "application/x-director"},
{".disco", "text/xml"},
{".divx", "video/divx"},
{".dll", "application/x-msdownload"},
{".dll.config", "text/xml"},
{".dlm", "text/dlm"},
{".doc", "application/msword"},
{".docm", "application/vnd.ms-word.document.macroEnabled.12"},
{".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
{".dot", "application/msword"},
{".dotm", "application/vnd.ms-word.template.macroEnabled.12"},
{".dotx", "application/vnd.openxmlformats-officedocument.wordprocessingml.template"},
{".dsp", "application/octet-stream"},
{".dsw", "text/plain"},
{".dtd", "text/xml"},
{".dtsConfig", "text/xml"},
{".dv", "video/x-dv"},
{".dvi", "application/x-dvi"},
{".dwf", "drawing/x-dwf"},
{".dwg", "application/acad"},
{".dxf", "application/x-dxf" },
{".dwp", "application/octet-stream"},
{".dxr", "application/x-director"},
{".eml", "message/rfc822"},
{".emz", "application/octet-stream"},
{".eot", "application/vnd.ms-fontobject"},
{".eps", "application/postscript"},
{".etl", "application/etl"},
{".etx", "text/x-setext"},
{".evy", "application/envoy"},
{".exe", "application/octet-stream"},
{".exe.config", "text/xml"},
{".fdf", "application/vnd.fdf"},
{".fif", "application/fractals"},
{".filters", "application/xml"},
{".fla", "application/octet-stream"},
{".flac", "audio/flac"},
{".flr", "x-world/x-vrml"},
{".flv", "video/x-flv"},
{".fsscript", "application/fsharp-script"},
{".fsx", "application/fsharp-script"},
{".generictest", "application/xml"},
{".gif", "image/gif"},
{".gpx", "application/gpx+xml"},
{".group", "text/x-ms-group"},
{".gsm", "audio/x-gsm"},
{".gtar", "application/x-gtar"},
-- {".gz", "application/x-gzip"},
{".h", "text/plain"},
{".hdf", "application/x-hdf"},
{".hdml", "text/x-hdml"},
{".hhc", "application/x-oleobject"},
{".hhk", "application/octet-stream"},
{".hhp", "application/octet-stream"},
{".hlp", "application/winhlp"},
{".hpp", "text/plain"},
{".hqx", "application/mac-binhex40"},
{".hta", "application/hta"},
{".htc", "text/x-component"},
{".htm", "text/html"},
{".html", "text/html"},
{".htt", "text/webviewhtml"},
{".hxa", "application/xml"},
{".hxc", "application/xml"},
{".hxd", "application/octet-stream"},
{".hxe", "application/xml"},
{".hxf", "application/xml"},
{".hxh", "application/octet-stream"},
{".hxi", "application/octet-stream"},
{".hxk", "application/xml"},
{".hxq", "application/octet-stream"},
{".hxr", "application/octet-stream"},
{".hxs", "application/octet-stream"},
{".hxt", "text/html"},
{".hxv", "application/xml"},
{".hxw", "application/octet-stream"},
{".hxx", "text/plain"},
{".i", "text/plain"},
{".ico", "image/x-icon"},
{".ics", "application/octet-stream"},
{".idl", "text/plain"},
{".ief", "image/ief"},
{".iii", "application/x-iphone"},
{".inc", "text/plain"},
{".inf", "application/octet-stream"},
{".ini", "text/plain"},
{".inl", "text/plain"},
{".ins", "application/x-internet-signup"},
{".ipa", "application/x-itunes-ipa"},
{".ipg", "application/x-itunes-ipg"},
{".ipproj", "text/plain"},
{".ipsw", "application/x-itunes-ipsw"},
{".iqy", "text/x-ms-iqy"},
{".isp", "application/x-internet-signup"},
{".ite", "application/x-itunes-ite"},
{".itlp", "application/x-itunes-itlp"},
{".itms", "application/x-itunes-itms"},
{".itpc", "application/x-itunes-itpc"},
{".IVF", "video/x-ivf"},
{".jar", "application/java-archive"},
{".java", "application/octet-stream"},
{".jck", "application/liquidmotion"},
{".jcz", "application/liquidmotion"},
{".jfif", "image/pjpeg"},
{".jnlp", "application/x-java-jnlp-file"},
{".jpb", "application/octet-stream"},
{".jpe", "image/jpeg"},
{".jpeg", "image/jpeg"},
{".jpg", "image/jpeg"},
{".js", "application/javascript"},
{".json", "application/json"},
{".jsx", "text/jscript"},
{".jsxbin", "text/plain"},
{".latex", "application/x-latex"},
{".library-ms", "application/windows-library+xml"},
{".lit", "application/x-ms-reader"},
{".loadtest", "application/xml"},
{".lpk", "application/octet-stream"},
{".lsf", "video/x-la-asf"},
{".lst", "text/plain"},
{".lsx", "video/x-la-asf"},
{".lzh", "application/octet-stream"},
{".m13", "application/x-msmediaview"},
{".m14", "application/x-msmediaview"},
{".m1v", "video/mpeg"},
{".m2t", "video/vnd.dlna.mpeg-tts"},
{".m2ts", "video/vnd.dlna.mpeg-tts"},
{".m2v", "video/mpeg"},
{".m3u", "audio/x-mpegurl"},
{".m3u8", "audio/x-mpegurl"},
{".m4a", "audio/m4a"},
{".m4b", "audio/m4b"},
{".m4p", "audio/m4p"},
{".m4r", "audio/x-m4r"},
{".m4v", "video/x-m4v"},
{".mac", "image/x-macpaint"},
{".mak", "text/plain"},
{".man", "application/x-troff-man"},
{".manifest", "application/x-ms-manifest"},
{".map", "text/plain"},
{".master", "application/xml"},
{".mbox", "application/mbox"},
{".mda", "application/msaccess"},
{".mdb", "application/x-msaccess"},
{".mde", "application/msaccess"},
{".mdp", "application/octet-stream"},
{".me", "application/x-troff-me"},
{".mfp", "application/x-shockwave-flash"},
{".mht", "message/rfc822"},
{".mhtml", "message/rfc822"},
{".mid", "audio/mid"},
{".midi", "audio/mid"},
{".mix", "application/octet-stream"},
{".mk", "text/plain"},
{".mmf", "application/x-smaf"},
{".mno", "text/xml"},
{".mny", "application/x-msmoney"},
{".mod", "video/mpeg"},
{".mov", "video/quicktime"},
{".movie", "video/x-sgi-movie"},
{".mp2", "video/mpeg"},
{".mp2v", "video/mpeg"},
{".mp3", "audio/mpeg"},
{".mp4", "video/mp4"},
{".mp4v", "video/mp4"},
{".mpa", "video/mpeg"},
{".mpe", "video/mpeg"},
{".mpeg", "video/mpeg"},
{".mpf", "application/vnd.ms-mediapackage"},
{".mpg", "video/mpeg"},
{".mpp", "application/vnd.ms-project"},
{".mpv2", "video/mpeg"},
{".mqv", "video/quicktime"},
{".ms", "application/x-troff-ms"},
{".msg", "application/vnd.ms-outlook"},
{".msi", "application/octet-stream"},
{".mso", "application/octet-stream"},
{".mts", "video/vnd.dlna.mpeg-tts"},
{".mtx", "application/xml"},
{".mvb", "application/x-msmediaview"},
{".mvc", "application/x-miva-compiled"},
{".mxp", "application/x-mmxp"},
{".nc", "application/x-netcdf"},
{".nsc", "video/x-ms-asf"},
{".nws", "message/rfc822"},
{".ocx", "application/octet-stream"},
{".oda", "application/oda"},
{".odb", "application/vnd.oasis.opendocument.database"},
{".odc", "application/vnd.oasis.opendocument.chart"},
{".odf", "application/vnd.oasis.opendocument.formula"},
{".odg", "application/vnd.oasis.opendocument.graphics"},
{".odh", "text/plain"},
{".odi", "application/vnd.oasis.opendocument.image"},
{".odl", "text/plain"},
{".odm", "application/vnd.oasis.opendocument.text-master"},
{".odp", "application/vnd.oasis.opendocument.presentation"},
{".ods", "application/vnd.oasis.opendocument.spreadsheet"},
{".odt", "application/vnd.oasis.opendocument.text"},
{".oga", "audio/ogg"},
{".ogg", "audio/ogg"},
{".ogv", "video/ogg"},
{".ogx", "application/ogg"},
{".one", "application/onenote"},
{".onea", "application/onenote"},
{".onepkg", "application/onenote"},
{".onetmp", "application/onenote"},
{".onetoc", "application/onenote"},
{".onetoc2", "application/onenote"},
{".opus", "audio/ogg"},
{".orderedtest", "application/xml"},
{".osdx", "application/opensearchdescription+xml"},
{".otf", "application/font-sfnt"},
{".otg", "application/vnd.oasis.opendocument.graphics-template"},
{".oth", "application/vnd.oasis.opendocument.text-web"},
{".otp", "application/vnd.oasis.opendocument.presentation-template"},
{".ots", "application/vnd.oasis.opendocument.spreadsheet-template"},
{".ott", "application/vnd.oasis.opendocument.text-template"},
{".oxt", "application/vnd.openofficeorg.extension"},
{".p10", "application/pkcs10"},
{".p12", "application/x-pkcs12"},
{".p7b", "application/x-pkcs7-certificates"},
{".p7c", "application/pkcs7-mime"},
{".p7m", "application/pkcs7-mime"},
{".p7r", "application/x-pkcs7-certreqresp"},
{".p7s", "application/pkcs7-signature"},
{".pbm", "image/x-portable-bitmap"},
{".pcast", "application/x-podcast"},
{".pct", "image/pict"},
{".pcx", "application/octet-stream"},
{".pcz", "application/octet-stream"},
{".pdf", "application/pdf"},
{".pfb", "application/octet-stream"},
{".pfm", "application/octet-stream"},
{".pfx", "application/x-pkcs12"},
{".pgm", "image/x-portable-graymap"},
{".pic", "image/pict"},
{".pict", "image/pict"},
{".pkgdef", "text/plain"},
{".pkgundef", "text/plain"},
{".pko", "application/vnd.ms-pki.pko"},
{".pls", "audio/scpls"},
{".pma", "application/x-perfmon"},
{".pmc", "application/x-perfmon"},
{".pml", "application/x-perfmon"},
{".pmr", "application/x-perfmon"},
{".pmw", "application/x-perfmon"},
{".png", "image/png"},
{".pnm", "image/x-portable-anymap"},
{".pnt", "image/x-macpaint"},
{".pntg", "image/x-macpaint"},
{".pnz", "image/png"},
{".pot", "application/vnd.ms-powerpoint"},
{".potm", "application/vnd.ms-powerpoint.template.macroEnabled.12"},
{".potx", "application/vnd.openxmlformats-officedocument.presentationml.template"},
{".ppa", "application/vnd.ms-powerpoint"},
{".ppam", "application/vnd.ms-powerpoint.addin.macroEnabled.12"},
{".ppm", "image/x-portable-pixmap"},
{".pps", "application/vnd.ms-powerpoint"},
{".ppsm", "application/vnd.ms-powerpoint.slideshow.macroEnabled.12"},
{".ppsx", "application/vnd.openxmlformats-officedocument.presentationml.slideshow"},
{".ppt", "application/vnd.ms-powerpoint"},
{".pptm", "application/vnd.ms-powerpoint.presentation.macroEnabled.12"},
{".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
{".prf", "application/pics-rules"},
{".prm", "application/octet-stream"},
{".prx", "application/octet-stream"},
{".ps", "application/postscript"},
{".psc1", "application/PowerShell"},
{".psd", "application/octet-stream"},
{".psess", "application/xml"},
{".psm", "application/octet-stream"},
{".psp", "application/octet-stream"},
{".pst", "application/vnd.ms-outlook"},
{".pub", "application/x-mspublisher"},
{".pwz", "application/vnd.ms-powerpoint"},
{".qht", "text/x-html-insertion"},
{".qhtm", "text/x-html-insertion"},
{".qt", "video/quicktime"},
{".qti", "image/x-quicktime"},
{".qtif", "image/x-quicktime"},
{".qtl", "application/x-quicktimeplayer"},
{".qxd", "application/octet-stream"},
{".ra", "audio/x-pn-realaudio"},
{".ram", "audio/x-pn-realaudio"},
{".rar", "application/x-rar-compressed"},
{".ras", "image/x-cmu-raster"},
{".rat", "application/rat-file"},
{".rc", "text/plain"},
{".rc2", "text/plain"},
{".rct", "text/plain"},
{".rdlc", "application/xml"},
{".reg", "text/plain"},
{".resx", "application/xml"},
{".rf", "image/vnd.rn-realflash"},
{".rgb", "image/x-rgb"},
{".rgs", "text/plain"},
{".rm", "application/vnd.rn-realmedia"},
{".rmi", "audio/mid"},
{".rmp", "application/vnd.rn-rn_music_package"},
{".roff", "application/x-troff"},
{".rpm", "audio/x-pn-realaudio-plugin"},
{".rqy", "text/x-ms-rqy"},
{".rtf", "application/rtf"},
{".rtx", "text/richtext"},
{".rvt", "application/octet-stream" },
{".ruleset", "application/xml"},
{".s", "text/plain"},
{".safariextz", "application/x-safari-safariextz"},
{".scd", "application/x-msschedule"},
{".scr", "text/plain"},
{".sct", "text/scriptlet"},
{".sd2", "audio/x-sd2"},
{".sdp", "application/sdp"},
{".sea", "application/octet-stream"},
{".searchConnector-ms", "application/windows-search-connector+xml"},
{".setpay", "application/set-payment-initiation"},
{".setreg", "application/set-registration-initiation"},
{".settings", "application/xml"},
{".sgimb", "application/x-sgimb"},
{".sgml", "text/sgml"},
{".sh", "application/x-sh"},
{".shar", "application/x-shar"},
{".shtml", "text/html"},
{".sit", "application/x-stuffit"},
{".sitemap", "application/xml"},
{".skin", "application/xml"},
{".skp", "application/x-koan" },
{".sldm", "application/vnd.ms-powerpoint.slide.macroEnabled.12"},
{".sldx", "application/vnd.openxmlformats-officedocument.presentationml.slide"},
{".slk", "application/vnd.ms-excel"},
{".sln", "text/plain"},
{".slupkg-ms", "application/x-ms-license"},
{".smd", "audio/x-smd"},
{".smi", "application/octet-stream"},
{".smx", "audio/x-smd"},
{".smz", "audio/x-smd"},
{".snd", "audio/basic"},
{".snippet", "application/xml"},
{".snp", "application/octet-stream"},
{".sol", "text/plain"},
{".sor", "text/plain"},
{".spc", "application/x-pkcs7-certificates"},
{".spl", "application/futuresplash"},
{".spx", "audio/ogg"},
{".src", "application/x-wais-source"},
{".srf", "text/plain"},
{".SSISDeploymentManifest", "text/xml"},
{".ssm", "application/streamingmedia"},
{".sst", "application/vnd.ms-pki.certstore"},
{".stl", "application/vnd.ms-pki.stl"},
{".sv4cpio", "application/x-sv4cpio"},
{".sv4crc", "application/x-sv4crc"},
{".svc", "application/xml"},
{".svg", "image/svg+xml"},
{".swf", "application/x-shockwave-flash"},
{".step", "application/step"},
{".stp", "application/step"},
{".t", "application/x-troff"},
{".tar", "application/x-tar"},
{".tcl", "application/x-tcl"},
{".testrunconfig", "application/xml"},
{".testsettings", "application/xml"},
{".tex", "application/x-tex"},
{".texi", "application/x-texinfo"},
{".texinfo", "application/x-texinfo"},
{".tgz", "application/x-compressed"},
{".thmx", "application/vnd.ms-officetheme"},
{".thn", "application/octet-stream"},
{".tif", "image/tiff"},
{".tiff", "image/tiff"},
{".tlh", "text/plain"},
{".tli", "text/plain"},
{".toc", "application/octet-stream"},
{".tr", "application/x-troff"},
{".trm", "application/x-msterminal"},
{".trx", "application/xml"},
{".ts", "video/vnd.dlna.mpeg-tts"},
{".tsv", "text/tab-separated-values"},
{".ttf", "application/font-sfnt"},
{".tts", "video/vnd.dlna.mpeg-tts"},
{".txt", "text/plain"},
{".u32", "application/octet-stream"},
{".uls", "text/iuls"},
{".user", "text/plain"},
{".ustar", "application/x-ustar"},
{".vb", "text/plain"},
{".vbdproj", "text/plain"},
{".vbk", "video/mpeg"},
{".vbproj", "text/plain"},
{".vbs", "text/vbscript"},
{".vcf", "text/x-vcard"},
{".vcproj", "application/xml"},
{".vcs", "text/plain"},
{".vcxproj", "application/xml"},
{".vddproj", "text/plain"},
{".vdp", "text/plain"},
{".vdproj", "text/plain"},
{".vdx", "application/vnd.ms-visio.viewer"},
{".vml", "text/xml"},
{".vscontent", "application/xml"},
{".vsct", "text/xml"},
{".vsd", "application/vnd.visio"},
{".vsi", "application/ms-vsi"},
{".vsix", "application/vsix"},
{".vsixlangpack", "text/xml"},
{".vsixmanifest", "text/xml"},
{".vsmdi", "application/xml"},
{".vspscc", "text/plain"},
{".vss", "application/vnd.visio"},
{".vsscc", "text/plain"},
{".vssettings", "text/xml"},
{".vssscc", "text/plain"},
{".vst", "application/vnd.visio"},
{".vstemplate", "text/xml"},
{".vsto", "application/x-ms-vsto"},
{".vsw", "application/vnd.visio"},
{".vsx", "application/vnd.visio"},
{".vtx", "application/vnd.visio"},
{".wav", "audio/wav"},
{".wave", "audio/wav"},
{".wax", "audio/x-ms-wax"},
{".wbk", "application/msword"},
{".wbmp", "image/vnd.wap.wbmp"},
{".wcm", "application/vnd.ms-works"},
{".wdb", "application/vnd.ms-works"},
{".wdp", "image/vnd.ms-photo"},
{".webarchive", "application/x-safari-webarchive"},
{".webm", "video/webm"},
{".webp", "image/webp"}, // https://en.wikipedia.org/wiki/WebP
{".webtest", "application/xml"},
{".wiq", "application/xml"},
{".wiz", "application/msword"},
{".wks", "application/vnd.ms-works"},
{".WLMP", "application/wlmoviemaker"},
{".wlpginstall", "application/x-wlpg-detect"},
{".wlpginstall3", "application/x-wlpg3-detect"},
{".wm", "video/x-ms-wm"},
{".wma", "audio/x-ms-wma"},
{".wmd", "application/x-ms-wmd"},
{".wmf", "application/x-msmetafile"},
{".wml", "text/vnd.wap.wml"},
{".wmlc", "application/vnd.wap.wmlc"},
{".wmls", "text/vnd.wap.wmlscript"},
{".wmlsc", "application/vnd.wap.wmlscriptc"},
{".wmp", "video/x-ms-wmp"},
{".wmv", "video/x-ms-wmv"},
{".wmx", "video/x-ms-wmx"},
{".wmz", "application/x-ms-wmz"},
{".woff", "application/font-woff"},
{".wpl", "application/vnd.ms-wpl"},
{".wps", "application/vnd.ms-works"},
{".wri", "application/x-mswrite"},
{".wrl", "x-world/x-vrml"},
{".wrz", "x-world/x-vrml"},
{".wsc", "text/scriptlet"},
{".wsdl", "text/xml"},
{".wvx", "video/x-ms-wvx"},
{".x", "application/directx"},
{".xaf", "x-world/x-vrml"},
{".xaml", "application/xaml+xml"},
{".xap", "application/x-silverlight-app"},
{".xbap", "application/x-ms-xbap"},
{".xbm", "image/x-xbitmap"},
{".xdr", "text/plain"},
{".xht", "application/xhtml+xml"},
{".xhtml", "application/xhtml+xml"},
{".xla", "application/vnd.ms-excel"},
{".xlam", "application/vnd.ms-excel.addin.macroEnabled.12"},
{".xlc", "application/vnd.ms-excel"},
{".xld", "application/vnd.ms-excel"},
{".xlk", "application/vnd.ms-excel"},
{".xll", "application/vnd.ms-excel"},
{".xlm", "application/vnd.ms-excel"},
{".xls", "application/vnd.ms-excel"},
{".xlsb", "application/vnd.ms-excel.sheet.binary.macroEnabled.12"},
{".xlsm", "application/vnd.ms-excel.sheet.macroEnabled.12"},
{".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
{".xlt", "application/vnd.ms-excel"},
{".xltm", "application/vnd.ms-excel.template.macroEnabled.12"},
{".xltx", "application/vnd.openxmlformats-officedocument.spreadsheetml.template"},
{".xlw", "application/vnd.ms-excel"},
{".xml", "text/xml"},
{".xmp", "application/octet-stream" },
{".xmta", "application/xml"},
{".xof", "x-world/x-vrml"},
{".XOML", "text/plain"},
{".xpm", "image/x-xpixmap"},
{".xps", "application/vnd.ms-xpsdocument"},
{".xrm-ms", "text/xml"},
{".xsc", "application/xml"},
{".xsd", "text/xml"},
{".xsf", "text/xml"},
{".xsl", "text/xml"},
{".xslt", "text/xml"},
{".xsn", "application/octet-stream"},
{".xss", "application/xml"},
{".xspf", "application/xspf+xml"},
{".xtp", "application/octet-stream"},
{".xwd", "image/x-xwindowdump"},
{".z", "application/x-compress"},
{".zip", "application/zip"},

{"application/fsharp-script", ".fsx"},
{"application/msaccess", ".adp"},
{"application/msword", ".doc"},
{"application/octet-stream", ".bin"},
{"application/onenote", ".one"},
{"application/postscript", ".eps"},
{"application/step", ".step"},
{"application/vnd.ms-excel", ".xls"},
{"application/vnd.ms-powerpoint", ".ppt"},
{"application/vnd.ms-works", ".wks"},
{"application/vnd.visio", ".vsd"},
{"application/x-director", ".dir"},
{"application/x-shockwave-flash", ".swf"},
{"application/x-x509-ca-cert", ".cer"},
{"application/x-zip-compressed", ".zip"},
{"application/xhtml+xml", ".xhtml"},
{"application/xml", ".xml"},  // anomoly, .xml -> text/xml, but application/xml -> many thingss, but all are xml, so safest is .xml
{"audio/aac", ".AAC"},
{"audio/aiff", ".aiff"},
{"audio/basic", ".snd"},
{"audio/mid", ".midi"},
{"audio/wav", ".wav"},
{"audio/x-m4a", ".m4a"},
{"audio/x-mpegurl", ".m3u"},
{"audio/x-pn-realaudio", ".ra"},
{"audio/x-smd", ".smd"},
{"image/bmp", ".bmp"},
{"image/jpeg", ".jpg"},
{"image/pict", ".pic"},
{"image/png", ".png"},
{"image/tiff", ".tiff"},
{"image/x-macpaint", ".mac"},
{"image/x-quicktime", ".qti"},
{"message/rfc822", ".eml"},
{"text/html", ".html"},
{"text/plain", ".txt"},
{"text/scriptlet", ".wsc"},
{"text/xml", ".xml"},
{"video/3gpp", ".3gp"},
{"video/3gpp2", ".3gp2"},
{"video/mp4", ".mp4"},
{"video/mpeg", ".mpg"},
{"video/quicktime", ".mov"},
{"video/vnd.dlna.mpeg-tts", ".m2t"},
{"video/x-dv", ".dv"},
{"video/x-la-asf", ".lsf"},
{"video/x-ms-asf", ".asf"},
{"x-world/x-vrml", ".xof"},
*/

/*static*/int FASTCALL SFile::WildcardMatch(const char * pPattern, const char * pStr)
{
	//
	// Backtrack to previous * on mismatch and retry starting one
	// character later in the string.  Because * matches all characters
	// (no exception for /), it can be easily proved that there's never a need to backtrack multiple levels.
	//
	const char * back_pat = NULL;
	const char * back_str = pStr; //back_str;
	//
	// Loop over each token (character or class) in pat, matching
	// it against the remaining unmatched tail of str.  Return false
	// on mismatch, or true after matching the trailing nul bytes.
	//
	for(;;) {
		uchar c = *pStr++;
		uchar d = *pPattern++;
		switch(d) {
			case '?': // Wildcard: anything but nul
				if(c == '\0')
					return 0;
				break;
			case '*': // Any-length wildcard
				if(*pPattern == '\0') // Optimize trailing * case
					return 1;
				back_pat = pPattern;
				back_str = --pStr; // Allow zero-length match
				break;
			case '[': // Character class
				{
					const  int  inverted = BIN(*pPattern == '!');
					const  char * p_cls = pPattern + inverted;
					int    match = 0;
					uchar  a = *p_cls++;
					//
					// Iterate over each span in the character class.
					// A span is either a single character a, or a range a-b.  The first span may begin with ']'.
					//
					do {
						uchar b = a;
						if(a == '\0') // Malformed
							goto literal;
						if(p_cls[0] == '-' && p_cls[1] != ']') {
							b = p_cls[1];
							if(b == '\0')
								goto literal;
							p_cls += 2;
							// Any special action if a > b?
						}
						match |= (a <= c && c <= b);
					} while((a = *p_cls++) != ']');
					if(match == inverted)
						goto backtrack;
					pPattern = p_cls;
				}
				break;
			case '\\':
				d = *pPattern++;
				// @fallthrough
			default: // Literal character
literal:
				{
					if(c == d) {
						if(d == '\0')
							return 1;
						break;
					}
					else if(c > d && (c - d) == ('a' - 'A') && (c >= 'a' && c <= 'z')) {
						if(d == '\0')
							return 1;
						break;
					}
					else if(d > c && (d - c) == ('a' - 'A') && (d >= 'a' && d <= 'z')) {
						if(d == '\0')
							return 1;
						break;
					}
				}
backtrack:
				if(c == '\0' || !back_pat)
					return 0; // No point continuing
				// Try again from last *, one character later in str.
				pPattern = back_pat;
				pStr = ++back_str;
				break;
			}
	}
}

/*static*/int FASTCALL SFile::Remove(const char * pFileName)
{
	return isempty(pFileName) ? -1 : ((::remove(pFileName) == 0) ? 1 : SLS.SetError(SLERR_FILE_DELETE, pFileName));
}

/*static*/int SFile::Rename(const char * pFileName, const char * pNewFileName)
{
	return (::rename(pFileName, pNewFileName) == 0) ? 1 : SLS.SetError(SLERR_FILE_RENAME, pFileName);
}

/*static*/int SFile::Compare(const char * pFileName1, const char * pFileName2, long flags)
{
	int    ok = 1;
	int    r1 = 0, r2 = 0;
	const  size_t blk_size = 4096;
	size_t actual_size1 = 0, actual_size2 = 0;
	SFile f1(pFileName1, SFile::mRead|SFile::mBinary|SFile::mNoStd);
	SFile f2(pFileName2, SFile::mRead|SFile::mBinary|SFile::mNoStd);
	SBaseBuffer b1, b2;
	b1.Init();
	b2.Init();
	THROW(f1.IsValid());
	THROW(f2.IsValid());
	THROW(b1.Alloc(blk_size));
	THROW(b2.Alloc(blk_size));
	do {
		THROW(r1 = f1.Read(b1.P_Buf, b1.Size, &actual_size1));
		THROW(r2 = f2.Read(b2.P_Buf, b2.Size, &actual_size2));
		if(actual_size1 != actual_size2 || memcmp(b1.P_Buf, b2.P_Buf, actual_size1) != 0) {
			ok = -1;
			break;
		}
	} while(actual_size1 && actual_size2 && r1 > 0 && r2 > 0);
	CATCHZOK
	return ok;
}

static void FASTCALL wftime_to_ldatetime(FILETIME * wt, LDATETIME * st)
{
	uint16 d, t;
	FileTimeToLocalFileTime(wt, wt); // debug
	FileTimeToDosDateTime(wt, &d, &t);
	decode_fat_datetime(d, t, st);
}

static void FASTCALL ldatetime_to_wftime(const LDATETIME * st, FILETIME * wt)
{
	uint16 d, t;
	encode_fat_datetime(&d, &t, st);
	DosDateTimeToFileTime(d, t, wt);
	LocalFileTimeToFileTime(wt, wt); // debug
}

/*static*/int SFile::SetTime(int fh, const LDATETIME * pCreation, const LDATETIME * pLastAccess, const LDATETIME * pLastModif)
{
#ifdef __WIN32__
	FILETIME w_cr_ft;
	FILETIME w_la_ft;
	FILETIME w_lm_ft;
	ldatetime_to_wftime(pCreation,   &w_cr_ft);
	ldatetime_to_wftime(pLastAccess, &w_la_ft);
	ldatetime_to_wftime(pLastModif,  &w_lm_ft);
	return BIN(::SetFileTime(reinterpret_cast<HANDLE>(fh), &w_cr_ft, &w_la_ft, &w_lm_ft));
#else
	struct ftime ftm;
	int d, m, y, h, s, hs;
	decodedate(&d, &m, &y, &pLastModif->d);
	ftm.ft_day   = d;
	ftm.ft_month = m;
	ftm.ft_year  = y - 1980;
	decodetime(&h, &m, &s, &hs, &pLastModif->t);
	ftm.ft_hour = h;
	ftm.ft_min  = m;
	ftm.ft_tsec = s / 2;
	return BIN(setftime(fh, &ftm) == 0);
#endif
}

/*static*/int SFile::GetTime(const char * pFileName, LDATETIME * pCreation, LDATETIME * pLastAccess, LDATETIME * pLastModif)
{
	int    ok = 1;
	HANDLE handle = ::CreateFile(SUcSwitch(pFileName), FILE_READ_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0); // @unicodeproblem
	if(handle != INVALID_HANDLE_VALUE) {
		ok = SFile::GetTime((int)handle, pCreation, pLastAccess, pLastModif);
		::CloseHandle(handle);
	}
	else
		ok = 0;
	return ok;
}

/*static*/int SFile::GetTime(int fh, LDATETIME * creation, LDATETIME * lastAccess, LDATETIME * lastModif)
{
#ifdef __WIN32__
	FILETIME w_cr_ft;
	FILETIME w_la_ft;
	FILETIME w_lm_ft;
	if(::GetFileTime((HANDLE)fh, &w_cr_ft, &w_la_ft, &w_lm_ft)) {
		if(creation)
			wftime_to_ldatetime(&w_cr_ft, creation);
		if(lastAccess)
			wftime_to_ldatetime(&w_la_ft, lastAccess);
		if(lastModif)
			wftime_to_ldatetime(&w_lm_ft, lastModif);
		return 1;
	}
	else
		return 0;
#else
	struct ftime ftm;
	if(getftime(fh, &ftm) == 0) {
		lastModif->d = encodedate(ftm.ft_day, ftm.ft_month, ftm.ft_year + 1980);
		lastModif->t = encodetime(ftm.ft_hour, ftm.ft_min, ftm.ft_tsec * 2, 0);
		lastAccess->d = creation->d = lastModif->d;
		lastAccess->t = creation->t = lastModif->t;
		return 1;
	}
	else
		return 0;
#endif
}
//
// Returns:
//		1 - файл открыт для записи другим процессом
//		0 - файл доступен для открытия на чтение
//	   -1 - ошибка
//
/*static*/int SFile::IsOpenedForWriting(const char * pFileName)
{
	int    ok = 0;
	HANDLE handle = ::CreateFile(SUcSwitch(pFileName), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0); // @unicodeproblem
	// Если ошибка, то файл открыт
	if(handle == INVALID_HANDLE_VALUE) {
		if(GetLastError() == ERROR_SHARING_VIOLATION) {
			SLS.SetError(SLERR_FILESHARINGVIOLATION, pFileName);
			ok = 1;
		}
		else {
			ok = -1;
		}
	}
	else {
		CloseHandle(handle);
		ok = 0;
	}
    return ok;
}

/*static*/int SFile::WaitForWriteSharingRelease(const char * pFileName, long timeout)
{
	int    ok = -1;
	int    r = IsOpenedForWriting(pFileName);
	if(r == 0)
		ok = -1;
	else if(r < 0)
		ok = 0;
	else if(timeout > 0) {
		//const  LDATETIME dtm_start = getcurdatetime_();
		const  long c_start = clock();
		long   msec = 0;
		long   days = 0;
		do {
			SDelay(100);
			r = IsOpenedForWriting(pFileName);
			//LDATETIME dtm_cur = getcurdatetime_();
			//msec = diffdatetime(dtm_cur, dtm_start, 4, &(days = 0));
			const long c_cur = clock();
            msec = (c_cur - c_start);
		} while(r > 0 && (/*days == 0 &&*/ msec < timeout));
		ok = (r == 0) ? msec : 0;
	}
	return ok;
}

/*static*/void FASTCALL SFile::ZClose(FILE ** ppF)
{
	if(ppF && *ppF) {
		fclose(*ppF);
		*ppF = 0;
	}
}

void SFile::Init()
{
	T = tNone;
	F = 0;
	IH = -1;
	Mode = 0;
	Name = 0;
}

int SFile::InvariantC(SInvariantParam * pInvP) const
{
	S_INVARIANT_PROLOG(pInvP);
	S_ASSERT_P(T != tStdFile || F, pInvP);
	S_ASSERT_P(T != tFile   || IH != -1, pInvP);
	S_ASSERT_P(T != tSBuffer || P_Sb, pInvP);
	S_INVARIANT_EPILOG(pInvP);
}

SFile::SFile() : LB(0)
{
	Init();
}

SFile::SFile(const char * pName, long mode) : LB(0)
{
	Init();
	if(mode & mNullWrite)
		OpenNullOutput();
	else
		Open(pName, mode);
}

SFile::SFile(SBuffer & rBuf, long mode) : LB(0)
{
	Init();
	Open(rBuf, mode);
}

SFile::~SFile()
{
	Close();
}

SFile::operator FILE * () { return (T == tStdFile) ? F : 0; }
SFile::operator SBuffer * () { return (T == tSBuffer) ? P_Sb : 0; }
int   SFile::FileNo() const { return (T == tStdFile && F) ? fileno(F) : IH; }
const SString & SFile::GetName() const { return Name; }
long  SFile::GetMode() const { return Mode; }

int SFile::GetBuffer(SBaseBuffer & rBuf) const
{
	if(T == tSBuffer && P_Sb) {
		rBuf.P_Buf = (char *)P_Sb->GetBuf(0); // @attention @badcast
		rBuf.Size = P_Sb->GetAvailableSize();
		return 1;
	}
	else
		return 0;
}

bool SFile::IsValid() const
{
	assert(InvariantC(0));
	return F ? true : ((IH >= 0 || T == tNullOutput) ? true : SLS.SetError(SLERR_FILENOTOPENED));
}

int SFile::Open(SBuffer & rBuf, long mode)
{
	assert(InvariantC(0));
	int    ok = 1;
	Close();
	THROW_S(P_Sb = new SBuffer(rBuf), SLERR_NOMEM);
	T = tSBuffer;
	{
		const long m = (mode & ~(mBinary | mDenyRead | mDenyWrite | mNoStd));
		Mode = mode;
		if(m == mReadWriteTrunc)
			P_Sb->SetWrOffs(0);
	}
	CATCHZOK
	return ok;
}

int SFile::OpenNullOutput()
{
	Close();
	IH = -1;
	T = tNullOutput;
	return 1;
}

int SFile::Open(const char * pName, long mode)
{
	int    ok = 1;
	bool   err_inited = false; // @v11.3.3
	SString mode_buf;
	// @v10.6.0 const  long   m = (mode & ~(mBinary | mDenyRead | mDenyWrite | mNoStd | mNullWrite));
	int    oflag = 0;
	int    pflag = S_IREAD | S_IWRITE;
	int    shflag = 0;
	Mode = mode;
	switch(mode & ~(mBinary|mDenyRead|mDenyWrite|mNoStd|mNullWrite)) { // @v10.6.0
		case mRead:
			oflag |= O_RDONLY;
			mode_buf.CatChar('r');
			break;
		case mWrite:
			oflag |= (O_WRONLY | O_CREAT | O_TRUNC);
			mode_buf.CatChar('w');
			break;
		case mAppend:
			oflag |= (O_WRONLY | O_CREAT | O_APPEND);
			mode_buf.CatChar('a');
			break;
		case mReadWrite:
			oflag |= (O_RDWR | O_CREAT);
			mode_buf.CatChar('r').CatChar('+');
			break;
		case mReadWriteTrunc:
			oflag |= (O_RDWR | O_TRUNC | O_CREAT);
			mode_buf.CatChar('w').CatChar('+');
			break;
		case mAppendRead:
			oflag |= (O_RDWR | O_APPEND);
			mode_buf.CatChar('a').CatChar('+');
			break;
	}
	if(mode & mBinary) {
		oflag |= O_BINARY;
		mode_buf.CatChar('b');
	}
	else {
		oflag |= O_TEXT;
		mode_buf.CatChar('t');
	}
	if(mode & mDenyRead && mode & mDenyWrite)
		shflag = SH_DENYRW;
	else if(mode & mDenyRead)
		shflag = SH_DENYRD;
	else if(mode & mDenyWrite)
		shflag = SH_DENYWR;
	else
		shflag = SH_DENYNO;
	Close();
	IH = sopen(pName, oflag, shflag, pflag);
	if(IH >= 0) {
		if(!(mode & mNoStd)) {
			F = fdopen(IH, mode_buf);
			if(F)
				T = tStdFile;
			else {
				SLS.SetErrorErrno(pName); // @v11.3.3
				err_inited = true;
				close(IH);
				IH = -1;
			}
		}
		else
			T = tFile;
	}
	if(IsValid()) {
		Mode = mode;
		Name = pName;
		ok = 1;
	}
	else {
		ok = err_inited ? 0 : SLS.SetError(SLERR_OPENFAULT, pName);
		Init();
	}
	return ok;
}

int SFile::Close()
{
	assert(InvariantC(0));
	int    ok = 1;
	if(T == tSBuffer) {
		ZDELETE(P_Sb);
		Init();
	}
	else {
		for(uint i = 0; i < LckList.getCount(); i++) {
			LckChunk & r_lck = LckList.at(i);
			assert(r_lck.Offs >= 0 && r_lck.Size >= 0);
			if(r_lck.Size > 0) {
				Unlock((int)(i+1));
			}
		}
		if(F) {
			fclose(F);
			Init();
		}
		else if(IH >= 0) {
			close(IH);
			Init();
		}
		else
			ok = -1;
	}
	T = tNone;
	return ok;
}

int SFile::Seek(long offs, int origin)
{
	int    ok = 1;
	assert(InvariantC(0));
	if(T == tSBuffer) {
		long o;
		if(origin == SEEK_CUR)
			o = P_Sb->GetRdOffs() + offs;
		else if(origin == SEEK_END)
			o = P_Sb->GetWrOffs() + offs;
		else
			o = offs;
		if(checkirange(o, 0L, static_cast<long>(P_Sb->GetWrOffs()))) {
			P_Sb->SetRdOffs(o);
			ok = 1;
		}
		else
			ok = 0;
	}
	else {
		if(F)
			ok = BIN(fseek(F, offs, origin) == 0);
		else if(IH >= 0)
			ok = BIN(lseek(IH, offs, origin) >= 0);
		else
			ok = 0;
	}
	BufR.Z();
	return ok;
}

int SFile::Seek64(int64 offs, int origin)
{
	assert(InvariantC(0));
	int   ok = (T == tFile) ? BIN(IH >= 0 && _lseeki64(IH, offs, origin) >= 0) : Seek((long)offs, origin);
	BufR.Z();
	return ok;
}

long SFile::Tell()
{
	assert(InvariantC(0));
	long   t = 0;
	if(T == tSBuffer) {
		t = static_cast<long>(P_Sb->GetRdOffs());
	}
	else {
		if(F)
			t = ftell(F);
		else if(IH >= 0) {
			t = tell(IH);
			if(t >= 0) {
				const long bo = static_cast<long>(BufR.GetWrOffs());
				assert(bo <= t);
				if(bo <= t)
					t -= bo;
			}
		}
		else
			t = (SLibError = SLERR_FILENOTOPENED, 0);
	}
	return t;
}

int64 SFile::Tell64()
{
	assert(InvariantC(0));
	//return (T == tFile) ? ((IH >= 0) ? _telli64(IH) : 0) : (int64)Tell();
	int64 t = 0;
	if(T == tSBuffer) {
		t = static_cast<int64>(P_Sb->GetRdOffs());
	}
	else {
		if(F)
			t = ftell(F);
		else if(IH >= 0) {
			t = _telli64(IH);
			if(t >= 0) {
				const int64 bo = static_cast<int64>(BufR.GetWrOffs());
				assert(bo <= t);
				if(bo <= t)
					t -= bo;
			}
		}
		else
			t = (SLibError = SLERR_FILENOTOPENED, 0);
	}
	return t;
}

int SFile::Flush()
{
	int    ok = -1;
	if(T == tStdFile) {
		if(F)
			ok = BIN(fflush(F) == 0);
	}
	return ok;
}

int SFile::Write(const void * pBuf, size_t size)
{
	assert(InvariantC(0));
	int    ok = 1;
	if(T == tNullOutput)
		ok = 1;
	else if(T == tSBuffer)
		ok = P_Sb->Write(pBuf, size);
	else if(F)
		ok = (fwrite(pBuf, size, 1, F) == 1) ? 1 : SLS.SetError(SLERR_WRITEFAULT, Name);
	else if(IH >= 0)
		ok = (write(IH, pBuf, size) == size) ? 1 : SLS.SetError(SLERR_WRITEFAULT, Name);
	else
		ok = (SLibError = SLERR_FILENOTOPENED, 0);
	return ok;
}

int SFile::ReadV(void * pBuf, size_t size)
{
	int    ok = 1;
	if(T == tNullOutput)
		ok = SLS.SetError(SLERR_SFILRDNULLOUTP, 0);
	else {
		size_t actual_size = 0;
		int    ok = Read(pBuf, size, &actual_size);
		if(ok && actual_size != size) {
			SString msg_buf;
			ok = SLS.SetError(SLERR_SFILRDSIZE, msg_buf.Cat(size).Cat("<<").Cat(actual_size));
		}
	}
	return ok;
}

int SFile::Read(void * pBuf, size_t size, size_t * pActualSize)
{
	assert(InvariantC(0));
	int    ok = 1;
	int    act_size = 0;
	THROW_S(T != tNullOutput, SLERR_SFILRDNULLOUTP);
	if(T == tSBuffer) {
		act_size = P_Sb->Read(pBuf, size);
	}
	else {
		THROW_S_S(F || (IH >= 0), SLERR_FILENOTOPENED, Name);
		if(F) {
			const int64 offs = Tell64();
			if(fread(pBuf, size, 1, F) == 1)
				act_size = (int)size;
			else {
				//
				// Для того чтобы функция считала последний блок из файла, если он не равен size
				//
				Seek64(offs, SEEK_SET);
				act_size = (int)fread(pBuf, 1, size, F);
				if(!act_size) {
					if(feof(F)) // @v9.7.10 @fix test for EOF added
						ok = -1;
					else
						CALLEXCEPT_S_S(SLERR_READFAULT, Name);
				}
			}
		}
		else if(IH >= 0) {
			size_t size_to_do = size;
			int    is_eof = 0;
			while(ok > 0 && size_to_do) {
				const size_t br_size = BufR.GetAvailableSize();
				if(br_size) {
					const size_t local_size_to_read = MIN(size_to_do, br_size);
					const size_t local_act_size = BufR.Read(pBuf, local_size_to_read);
					THROW_S_S(local_act_size == local_size_to_read, SLERR_READFAULT, Name);
					act_size += local_act_size;
					size_to_do -= local_act_size;
				}
				if(size_to_do) {
					if(is_eof)
						ok = -1;
					else if(Mode & SFile::mBuffRd) {
						const size_t SFile_MaxRdBuf_Size = 1024 * 1024;
						assert(!is_eof); // Флаг должен был быть установлен на предыдущей итерации. Если мы сюда попали с этим флагом, значит что-то не так!
						assert(SFile_MaxRdBuf_Size > BufR.GetAvailableSize());
						STempBuffer temp_buf(SFile_MaxRdBuf_Size - BufR.GetAvailableSize());
						THROW(temp_buf.IsValid());
						const int local_act_size = read(IH, temp_buf, temp_buf.GetSize());
						THROW_S_S(local_act_size >= 0, SLERR_READFAULT, Name);
						THROW(BufR.Write(temp_buf, local_act_size));
						if(local_act_size < (int)temp_buf.GetSize()) {
							is_eof = 1;
							if(local_act_size == 0)
								ok = -1;
						}
					}
					else {
						const int local_act_size = read(IH, pBuf, size_to_do);
						THROW_S_S(local_act_size >= 0, SLERR_READFAULT, Name);
						act_size += local_act_size;
						if(local_act_size < (int)size_to_do) {
							is_eof = 1;
							ok = -1;
						}
						size_to_do -= local_act_size;
					}
				}
			}
		}
	}
	CATCH
		act_size = 0;
		ok = 0;
	ENDCATCH
	ASSIGN_PTR(pActualSize, (size_t)act_size);
	return ok;
}

int SFile::WriteBlancLine()
{
	return WriteLine(0);
}

int FASTCALL SFile::WriteLine(const char * pBuf)
{
	assert(InvariantC(0));
	int    ok = 1;
	if(T == tNullOutput) {
		;
	}
	else {
		char   temp_buf[16];
		size_t size_to_write = 0;
		if(pBuf) {
			size_to_write = sstrlen(pBuf);
		}
		else {
			if(Mode & mBinary) {
				temp_buf[0] = '\x0D';
				temp_buf[1] = '\x0A';
				temp_buf[2] = 0;
				size_to_write = 2;
			}
			else {
				temp_buf[0] = '\n';
				temp_buf[1] = 0;
				size_to_write = 1;
			}
			pBuf = temp_buf;
		}
		if(T == tSBuffer)
			ok = P_Sb->Write(pBuf, size_to_write);
		else if(F)
			ok = (fputs(pBuf, F) >= 0) ? 1 : SLS.SetError(SLERR_WRITEFAULT, Name);
		else if(IH >= 0) {
			// @v9.5.10 ok = Write(pBuf, size_to_write);
			ok = (write(IH, pBuf, size_to_write) == size_to_write) ? 1 : SLS.SetError(SLERR_WRITEFAULT, Name); // @v9.5.10
		}
		else
			ok = (SLibError = SLERR_FILENOTOPENED, 0);
	}
	return ok;
}

int FASTCALL SFile::ReadLine(SString & rBuf)
{
	assert(InvariantC(0));
	int    ok = 1;
	rBuf.Z();
	THROW_S(T != tNullOutput, SLERR_SFILRDNULLOUTP);
	if(T == tStdFile) {
		THROW_S(F, SLERR_FILENOTOPENED);
		{
			THROW(LB.Alloc(1024));
			//
			// На случай, если строка в файле длиннее, чем buf_size
			// считываем ее в цикле до тех пор, пока в конце строки не появится //
			// перевод каретки.
			//
			char * p = 0;
			while((p = fgets(LB, LB.GetSize(), F)) != 0) {
				rBuf.Cat(LB);
				size_t len = sstrlen(LB);
				if(LB[len-1] == '\n' || (LB[len-2] == 0x0D && LB[len-1] == 0x0A))
					break;
			}
			THROW_S_S(rBuf.Len() || p, SLERR_READFAULT, Name);
		}
	}
	else if(oneof2(T, tSBuffer, tFile)) {
		THROW_S(T != tFile || IH >= 0, SLERR_FILENOTOPENED);
		if(IH >= 0 && Mode & SFile::mBuffRd) {
			int   rr = 0;
			int   _done = 0;
            do {
            	int8  rd_buf[16];
				size_t act_size = 0;
				THROW(rr = Read(rd_buf, 1, &act_size));
				if(act_size) {
					if(rd_buf[act_size-1] == '\n' || (rd_buf[act_size-1] == '\xA' && rBuf.Last() == '\xD'))
                        _done = 1;
					rBuf.CatN(reinterpret_cast<const char *>(rd_buf), act_size);
				}
            } while(rr > 0 && !_done);
            THROW_S_S(rBuf.Len(), SLERR_READFAULT, Name);
		}
		else {
			int64  last_pos = Tell64();
			char * p = 0;
			THROW(LB.Alloc(1024+2));
			size_t act_size = 0;
			LB[LB.GetSize()-2] = 0;
			while(Read(LB, LB.GetSize()-2, &act_size) && act_size) {
				p = static_cast<char *>(memchr(LB.vptr(), '\n', act_size));
				if(p) {
					p[1] = 0;
				}
				else {
					p = static_cast<char *>(memchr(LB.vptr(), 0x0D, act_size));
					// @v10.4.1 @fix 
					if(p) {
						if(p[1] == 0x0A)
							p[2] = 0;
						else
							p[1] = 0;
					}
				}
				if(p) {
					const size_t _len = sstrlen(LB.cptr());
					rBuf.CatN(LB, _len);
					Seek64(last_pos + _len); // @v9.5.9
					break;
				}
				else {
					LB[act_size] = 0;
					rBuf.Cat(LB);
					if(act_size < (LB.GetSize()-2))
						break;
				}
				last_pos = Tell64();
			}
			THROW_S_S(rBuf.Len() || p, SLERR_READFAULT, Name);
		}
	}
	else
		ok = 0;
	CATCHZOK
	return ok;
}

int SFile::Write(const SBuffer & rBuf)
{
	const size_t offs = rBuf.GetRdOffs();
	const uint32 size = rBuf.GetAvailableSize();
	return (Write(&size, sizeof(size)) && Write(rBuf.GetBuf(offs), size));
}

int SFile::Read(SBuffer & rBuf)
{
	int    ok = 1;
	uint32 size = 0;
	STempBuffer temp_buf(0);
	THROW(Read(&size, sizeof(size)));
	THROW(temp_buf.Alloc(size));
	THROW(Read(temp_buf, size));
	THROW(rBuf.Write(temp_buf, size));
	CATCHZOK
	return ok;
}
//
//
//
int SFile::AcquireLckDescriptor(int64 offs, int32 size)
{
	assert(size > 0);
	assert(offs >= 0);
	int    h = 0;
	long   zero = 0;
	uint   pos = 0;
	THROW(size > 0);
	THROW(offs >= 0);
	if(LckList.lsearch(&zero, &pos, CMPF_LONG)) {
		LckChunk & r_item = LckList.at(pos);
		r_item.Size = size;
		r_item.Offs = offs;
		h = (int)(pos+1);
	}
	else {
		LckChunk item;
		item.Size = size;
		item.Offs = offs;
		THROW(LckList.insert(&item));
		h = (int)LckList.getCount();
	}
	CATCH
		h = 0;
	ENDCATCH
	return h;
}

const SFile::LckChunk * FASTCALL SFile::GetLckDescriptor(int h) const
{
	const LckChunk * p_ret = 0;
	if(h > 0 && h <= LckList.getCountI()) {
		const LckChunk & r_item = LckList.at(h-1);
		assert(r_item.Size >= 0);
		if(r_item.Size > 0) {
			p_ret = &r_item;
		}
	}
	return p_ret;
}

int FASTCALL SFile::ReleaseLckDescriptor(int h)
{
	int    ok = 1;
	if(h > 0 && h <= LckList.getCountI()) {
		LckChunk & r_item = LckList.at(h-1);
		assert(r_item.Size >= 0);
		r_item.Size = 0;
	}
	else
		ok = 0;
	return ok;
}

int SFile::_Lock(int64 offs, int32 size, int mode)
{
	int    handle = 0;
	int    r = -1; // Результат выполнения _locking()
	assert(offs >= 0);
	assert(size > 0);
	assert(InvariantC(0));
	THROW(Seek64(offs, SEEK_SET));
	{
		int    _sm = mode ? LK_NBLCK : LK_UNLCK;
		r = _locking(IH, _sm, size);
		if(r == 0) {
			if(mode) {
				THROW(handle = AcquireLckDescriptor(offs, size));
			}
			else {
				handle = 1;
			}
		}
		else {
			SLS.SetError(SLERR_FLOCKFAULT, Name);
		}
	}
	CATCH
		if(mode) {
			if(r == 0) {
				_locking(IH, LK_UNLCK, size);
			}
			ReleaseLckDescriptor(handle);
		}
		handle = 0;
	ENDCATCH
	return handle;
}

int SFile::Lock(int64 offs, int32 size)
{
	return _Lock(offs, size, 1);
}

int SFile::Unlock(int lckHandle)
{
	const  LckChunk * p_lck = GetLckDescriptor(lckHandle);
	int    ret = p_lck ? _Lock(p_lck->Offs, p_lck->Size, 0) : 0;
	ReleaseLckDescriptor(lckHandle);
	return ret;
}

int SFile::CalcSize(int64 * pSize)
{
	assert(InvariantC(0));
	int    ok = 1;
	int64  sz = 0;
	if(T == tSBuffer)
		sz = P_Sb->GetWrOffs();
	else {
		if(IsValid()) {
			const int64  save_pos = Tell64();
			Seek64(0, SEEK_END);
			sz = Tell64();
			Seek64(save_pos, SEEK_SET);
		}
		else
			ok = 0;
	}
	ASSIGN_PTR(pSize, sz);
	return ok;
}

int SFile::GetDateTime(LDATETIME * pCreate, LDATETIME * pLastAccess, LDATETIME * pModif)
{
	if(T == tSBuffer)
		return -1;
	else
		return IsValid() ? SFile::GetTime(_get_osfhandle(fileno(F)), pCreate, pLastAccess, pModif) : 0;
}

int SFile::CalcCRC(long offs, uint32 * pCrc)
{
	int    ok = 1;
	SCRC32 c;
	uint32 crc = 0;
	int64  save_pos = Tell64();
	Seek64(0, SEEK_END);
	int64  sz = Tell64() - offs;
	if(sz > 0) {
		const  long  blk_size = 32*1024;
		int64  num_blk = sz / blk_size;
		long   rest = (long)(sz % blk_size);
		STempBuffer temp_buf(blk_size);
		Seek64(offs, SEEK_SET);
		for(int64 i = 0; i < num_blk; i++) {
			THROW(Read(temp_buf, temp_buf.GetSize()));
			crc = c.Calc(crc, temp_buf.cptr(), temp_buf.GetSize());
		}
		if(rest > 0) {
			THROW(Read(temp_buf, rest));
			crc = c.Calc(crc, temp_buf.cptr(), rest);
		}
	}
	else
		ok = -1;
	CATCHZOK
	Seek64(save_pos, SEEK_SET);
	ASSIGN_PTR(pCrc, crc);
	return ok;
}
//
// Идентификация форматов файлов
//
class FileFormatRegBase : private SVector, public SStrGroup { // @v9.8.10 SArray-->SVector
public:
	FileFormatRegBase();
	~FileFormatRegBase();
	int    Register(int id, int mimeType, const char * pMimeSubtype, const char * pExt, const char * pSign);
	int    Register(int id, int mimeType, const char * pMimeSubtype, const char * pExt, FileFormatSignatureFunc signFunc);
	//
	// Descr: Идентифицирует формат файла по расширению и сигнатуре.
	// Returns:
	//   1 - формат идентифицирован по расширению
	//   2 - формат идентифицироан по сигнатуре
	//   3 - формат идентифицирован одновременно по расширению и сигнатуре
	//   4 - формат не удалось идентифицировать по расширению или сигнатуре, однако
	//     по начальному блоку данных он похож на результирующий формат.
	//     На текущий момент такой вариант возможен для форматов: SFileFormat::TxtAscii, SFileFormat::TxtUtf8, SFileFormat::Txt
	//  -1 - не удалось идентифицировать формат
	//
	int    Identify(const char * pFileName, int * pFmtId, SString * pExt) const;
	int    IdentifyMime(const char * pMime, int * pFmtId) const;
	int    GetMime(int id, SString & rMime) const;
	int    GetExt(int id, SString & rExt) const;
private:
	struct Entry { // @flat
		int    FmtId;
		uint   ExtIdx;
		uint   SignIdx;
		int    MimeType; // SFileFormat::mtXXX
		uint   MimeSubtypeIdx;
		FileFormatSignatureFunc SignFunc;
	};
	int    SearchEntryByID(int id, LongArray & rPosList) const;
	int    SearchEntryByExt(const char * pExt, LongArray & rPosList) const;
	int    Get(uint pos, Entry & rEntry, SString & rExt, SString & rSign, int & rMimeType, SString & rMimeSubtype) const;
	int    Helper_Register(int id, int mimeType, const char * pMimeSubtype, const char * pExt, const char * pSign, FileFormatSignatureFunc signFunc);

	uint   MaxSignSize;
};

FileFormatRegBase::FileFormatRegBase() : SVector(sizeof(Entry)), MaxSignSize(0)
{
}

FileFormatRegBase::~FileFormatRegBase()
{
}

int FileFormatRegBase::GetMime(int id, SString & rMime) const
{
	rMime.Z();
	int    ok = 0;
	// @v10.0.05 {
	for(uint pos = 0; !ok && SVector::lsearch(&id, &pos, CMPF_LONG); pos++) {
		const Entry * p_entry = static_cast<const Entry *>(at(pos));
		assert(p_entry->FmtId == id);
		if(p_entry->MimeType && p_entry->MimeSubtypeIdx) {
			SFileFormat::GetMimeTypeName(p_entry->MimeType, rMime);
			SString & r_temp_buf = SLS.AcquireRvlStr();
			GetS(p_entry->MimeSubtypeIdx, r_temp_buf);
			rMime.CatChar('/').Cat(r_temp_buf);
			ok = 1;
		}
		// @v10.2.12 @fix break;
	}
	// } @v10.0.05 
	/* @v10.0.05
	for(uint i = 0; !ok && i < getCount(); i++) {
		const Entry * p_entry = static_cast<const Entry *>(at(i));
		if(p_entry && p_entry->FmtId == id) {
			if(p_entry->MimeType && p_entry->MimeSubtypeIdx) {
				SFileFormat::GetMimeTypeName(p_entry->MimeType, rMime);
				SString temp_buf;
				GetS(p_entry->MimeSubtypeIdx, temp_buf);
				rMime.CatChar('/').Cat(temp_buf);
				ok = 1;
			}
			break;
		}
	}
	*/
	return ok;
}

int FileFormatRegBase::GetExt(int id, SString & rExt) const
{
	int    ok = 0;
	SString temp_buf;
	for(uint i = 0; !ok && i < getCount(); i++) {
		const Entry * p_entry = static_cast<const Entry *>(at(i));
		if(p_entry && p_entry->FmtId == id) {
			if(p_entry->ExtIdx && GetS(p_entry->ExtIdx, temp_buf)) {
				// В регистрационной записи может быть несколько расширений, разделенных ';' - берем первое
				StringSet ss(';', temp_buf);
				if(ss.get(0U, temp_buf)) {
					rExt = temp_buf;
					ok = 1;
				}
			}
			break;
		}
	}
	return ok;
}

int FileFormatRegBase::SearchEntryByID(int id, LongArray & rPosList) const
{
	int    ok = -1;
	for(uint i = 0; i < getCount(); i++) {
		const Entry * p_entry = static_cast<const Entry *>(at(i));
		if(p_entry && p_entry->FmtId == id) {
			rPosList.add(i);
			ok = 1;
		}
	}
	return ok;
}

int FileFormatRegBase::SearchEntryByExt(const char * pExt, LongArray & rPosList) const
{
	int    ok = -1;
	SString ext(pExt);
	SString temp_buf;
	ext.Strip().ShiftLeftChr('.').Strip();
	for(uint i = 0; i < getCount(); i++) {
		const Entry * p_entry = static_cast<const Entry *>(at(i));
		if(p_entry && p_entry->ExtIdx) {
			if(GetS(p_entry->ExtIdx, temp_buf) && temp_buf == ext) {
				rPosList.add(i);
				ok = 1;
			}
		}
	}
	return ok;
}

int FileFormatRegBase::Get(uint pos, Entry & rEntry, SString & rExt, SString & rSign, int & rMimeType, SString & rMimeSubtype) const
{
	int    ok = 0;
	rExt.Z();
	rSign.Z();
	rMimeType = 0;
	rMimeSubtype.Z();
	if(pos < getCount()) {
		const Entry * p_entry = static_cast<const Entry *>(at(pos));
		if(p_entry) {
			SString temp_buf;
			rEntry = *p_entry;
			if(p_entry->ExtIdx && GetS(p_entry->ExtIdx, temp_buf)) {
				rExt = temp_buf;
			}
			if(p_entry->SignIdx && GetS(p_entry->SignIdx, temp_buf)) {
				rSign = temp_buf;
			}
			rMimeType = p_entry->MimeType;
			if(p_entry->MimeSubtypeIdx && GetS(p_entry->MimeSubtypeIdx, temp_buf)) {
				rMimeSubtype = temp_buf;
			}
			ok = 1;
		}
	}
	return ok;
}

int FileFormatRegBase::Helper_Register(int id, int mimeType, const char * pMimeSubtype, const char * pExt, const char * pSign, FileFormatSignatureFunc signFunc)
{
	int    ok = 1;
	SString new_ext;
	SString new_sign;
	SString new_mime_subtype;
	SString temp_buf;
	LongArray pos_list;
	(new_ext = pExt).Strip();
	(new_mime_subtype = pMimeSubtype).Strip();
	if(new_ext.HasChr(';')) {
		StringSet ss(';', new_ext);
		for(uint i = 0; ss.get(&i, temp_buf);) {
			THROW(ok = Helper_Register(id, mimeType, pMimeSubtype, temp_buf, pSign, signFunc)); // @recursion
		}
	}
	else {
		int    is_text_sign = 0;
		uint   i;
		new_ext.ShiftLeftChr('.').Strip().ToLower();
		(new_sign = pSign).Strip();
		if(new_sign.Len() > 1 && new_sign.C(0) == 'T') {
			is_text_sign = 1; // Текстовая сигнатура
		}
		else {
			// @v11.3.4 THROW(new_sign.IsEmpty() || (new_sign.Len() & 0x01) == 0);
			new_sign.ToLower();
			for(i = 0; i < new_sign.Len(); i++) {
				const int c = new_sign.C(i);
				THROW(oneof2(c, ':', ' ') || ishex(c));
			}
		}
		if(SearchEntryByID(id, pos_list) > 0) {
			SString entry_ext;
			SString entry_sign;
			SString entry_mime_subtype;
			int    entry_mime_type;
			for(i = 0; i < pos_list.getCount(); i++) {
				Entry entry;
				if(Get((uint)pos_list.get(i), entry, entry_ext, entry_sign, entry_mime_type, entry_mime_subtype)) {
					if(entry_ext == new_ext && entry_sign == new_sign && entry.SignFunc == signFunc &&
						entry_mime_type == mimeType && entry_mime_subtype == new_mime_subtype) {
						ok = -1;
						break;
					}
				}
			}
		}
		if(ok > 0) {
			Entry new_entry;
			MEMSZERO(new_entry);
			THROW(AddS(new_ext, &new_entry.ExtIdx));
			if(new_sign.NotEmpty()) {
				SETMAX(MaxSignSize, is_text_sign ? (new_sign.Len()-1) : new_sign.Len()/2);
				THROW(AddS(new_sign, &new_entry.SignIdx));
			}
			new_entry.MimeType = mimeType;
			THROW(AddS(new_mime_subtype, &new_entry.MimeSubtypeIdx));
			new_entry.FmtId = id;
			new_entry.SignFunc = signFunc;
			THROW(insert(&new_entry));
		}
	}
	CATCHZOK
	return ok;
}

int FileFormatRegBase::Register(int id, int mimeType, const char * pMimeSubtype, const char * pExt, const char * pSign)
	{ return Helper_Register(id, mimeType, pMimeSubtype, pExt, pSign, 0); }
int FileFormatRegBase::Register(int id, int mimeType, const char * pMimeSubtype, const char * pExt, FileFormatSignatureFunc signFunc)
	{ return Helper_Register(id, mimeType, pMimeSubtype, pExt, 0, signFunc); }

int FileFormatRegBase::IdentifyMime(const char * pMime, int * pFmtId) const
{
	int    ok = -1;
	int    fmt_id = 0;
	if(!isempty(pMime)) {
		SString entry_ext, entry_mime_subtype;
		SString temp_buf;
		(temp_buf = pMime).Strip();
		if(temp_buf.Divide('/', entry_ext, entry_mime_subtype) > 0) {
			const int key_mime_type = SFileFormat::IdentifyMimeType(entry_ext);
			if(key_mime_type && entry_mime_subtype.NotEmpty()) {
				const SString key_mime_subtype = entry_mime_subtype;
				for(uint i = 0; ok < 0 && i < getCount(); i++) {
					Entry entry;
					int   mime_type;
					if(Get(i, entry, entry_ext, temp_buf, mime_type, entry_mime_subtype)) {
						if(mime_type == key_mime_type && entry_mime_subtype.CmpNC(key_mime_subtype) == 0) {
							fmt_id = entry.FmtId;
							ok = 8;
						}
					}
				}
			}
		}
	}
	ASSIGN_PTR(pFmtId, fmt_id);
	return ok;
}

int FileFormatRegBase::Identify(const char * pFileName, int * pFmtId, SString * pExt) const
{
	int    ok = -1;
	int    fmt_id = 0;
	LongArray candid_by_ext;
	LongArray candid_by_sign;
	{
		SPathStruc ps(pFileName);
		int    entry_mime_type;
		SString ext = ps.Ext;
		SString entry_ext;
		SString entry_sign;
		SString entry_mime_subtype;
		SString temp_buf;
		SString left_buf;
		SString right_buf;
		StringSet ss_subsigns;
		LongArray used_offs_list;
		StrAssocArray binary_chunk_list;

		ASSIGN_PTR(pExt, ext);

		STempBuffer sign_buf(512);
		SFile file(pFileName, SFile::mRead|SFile::mBinary);

		int64  _fsize = 0;
		if(file.IsValid())
			file.CalcSize(&_fsize);
		ext.Strip().ShiftLeftChr('.').Strip().ToLower();
		for(uint i = 0; i < getCount(); i++) {
			Entry entry;
			if(Get(i, entry, entry_ext, entry_sign, entry_mime_type, entry_mime_subtype)) {
				if(entry_ext.NotEmpty() && entry_ext == ext) {
					candid_by_ext.addUnique(entry.FmtId);
				}
				if(_fsize > 0) {
					if(entry_sign.NotEmpty()) {
                        if(entry_sign.C(0) == 'T') {
							entry_sign.ShiftLeft(1);
							const size_t len = 512;
							size_t actual_size = 0;
							THROW(sign_buf.Alloc(len));
							file.Seek(0);
							assert(len <= sign_buf.GetSize());
							if(len <= sign_buf.GetSize() && file.Read(sign_buf, len, &actual_size)) {
								int    r = -1;
								size_t j = 0;
								//"EFBBBF"
								if(sign_buf.ucptr()[0] == 0xEF && sign_buf.ucptr()[1] == 0xBB && sign_buf.ucptr()[2] == 0xBF) // BOM UTF8 
									j += 3;
								while(r < 0 && j < actual_size) {
									const char c = sign_buf.cptr()[j];
									if(oneof4(c, ' ', '\t', '\x0D', '\x0A')) {
                                        j++;
									}
									else if(entry_sign.CmpL(sign_buf+j, 1) == 0) // @v10.0.02 @fix sign_buf-->sign_buf+j
										r = 1;
									else
										r = 0;
								}
								if(r) {
									candid_by_sign.addUnique(entry.FmtId);
								}
							}
                        }
                        else {
							//
							// За пределами 512-байтовой зоны ничего читать не будем!
							//
							used_offs_list.clear();
							binary_chunk_list.Z();
							ss_subsigns.clear();
							if(entry_sign.HasChr(' ')) {
								entry_sign.Tokenize(" ", ss_subsigns);
								for(uint ssp = 0; ss_subsigns.get(&ssp, temp_buf);) {
									long offs = 0;
									if(temp_buf.Divide(':', left_buf, right_buf) > 0) {
										offs = left_buf.ToLong();
										temp_buf = right_buf;
									}
									if((offs + temp_buf.Len()/2) <= 512 && used_offs_list.addUnique(offs) > 0)
										binary_chunk_list.Add(offs, temp_buf);
								}
							}
							else
								binary_chunk_list.Add(0, entry_sign);
							size_t total_len = 0;
							{
								for(uint ci = 0; ci < binary_chunk_list.getCount(); ci++) {
									StrAssocArray::Item bcl_item = binary_chunk_list.at_WithoutParent(ci);
									size_t local_len = (size_t)(bcl_item.Id + sstrlen(bcl_item.Txt) / 2);
									SETMAX(total_len, local_len);
								}
							}
							if(checkirange(total_len, 1, 512)) {
								const size_t len = total_len;
								size_t actual_size = 0;
								THROW(sign_buf.Alloc(len));
								file.Seek(0);
								assert(len <= sign_buf.GetSize());
								if(len <= sign_buf.GetSize() && file.Read(sign_buf, len, &actual_size) && actual_size == len) {
									int r = 1;
									for(uint ci = 0; r && ci < binary_chunk_list.getCount(); ci++) {
										StrAssocArray::Item bcl_item = binary_chunk_list.at_WithoutParent(ci);
										const size_t item_len = sstrlen(bcl_item.Txt) / 2;
										const size_t offs = bcl_item.Id;
										for(size_t j = 0; r && j < item_len; j++) {
											const uint8 file_byte = PTR8C(sign_buf.vcptr())[offs+j];
											const uint8 sign_byte = (uint8)(hex(bcl_item.Txt[j*2]) * 16 + hex(bcl_item.Txt[j*2+1]));
											if(file_byte != sign_byte)
												r = 0;
										}
									}
									if(r) {
										candid_by_sign.addUnique(entry.FmtId);
									}
								}
							}
                        }
					}
					else if(entry.SignFunc) {
						if(entry.SignFunc(file, 0) > 0) {
							candid_by_sign.addUnique(entry.FmtId);
						}
					}
				}
			}
		}
		{
			for(uint cidx = 0; !fmt_id && cidx < candid_by_ext.getCount(); cidx++) {
				const long ext_id = candid_by_ext.get(cidx);
				for(uint j = 0; !fmt_id && j < candid_by_sign.getCount(); j++)
					if(ext_id == candid_by_sign.get(j))
						fmt_id = ext_id;
			}
			if(fmt_id)
				ok = 3;
			else if(candid_by_sign.getCount()) {
				fmt_id = candid_by_sign.get(0);
				ok = 2;
			}
			else if(candid_by_ext.getCount()) {
				fmt_id = candid_by_ext.get(0);
				ok = 1;
			}
			else if(_fsize > 0) {
				STextEncodingStat tes;
				size_t actual_size = 0;
				STempBuffer tbuf(1024);
				if(tbuf.IsValid() && file.Read(tbuf, tbuf.GetSize(), &actual_size)) {
					tes.Add(tbuf, actual_size);
					tes.Finish();
					if(tes.CheckFlag(tes.fAsciiOnly)) {
						fmt_id = SFileFormat::TxtAscii;
						ok = 4;
					}
					else if(tes.CheckFlag(tes.fLegalUtf8Only)) {
						fmt_id = SFileFormat::TxtUtf8;
						ok = 4;
					}
					else if(tes.GetEolFormat() != eolUndef && !tes.CheckFlag(tes.fMiscEolf)) {
						fmt_id = SFileFormat::Txt;
						ok = 4;
					}
				}
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pFmtId, fmt_id);
	return ok;
}

/*static*/uint SFileFormat::GloBaseIdx = 0;

/*static*/int FASTCALL SFileFormat::Register(int id, const char * pExt, const char * pSign)
{
	return Register(id, 0, 0, pExt, pSign);
}

/*static*/int FASTCALL SFileFormat::Register(int id, int mimeType, const char * pMimeSubtype, const char * pExt, const char * pSign)
{
	int    ok = 0;
	if(!GloBaseIdx) {
		ENTER_CRITICAL_SECTION
		if(!GloBaseIdx) {
			TSClassWrapper <FileFormatRegBase> cls;
			GloBaseIdx = SLS.CreateGlobalObject(cls);
		}
		LEAVE_CRITICAL_SECTION
	}
	if(GloBaseIdx) {
		FileFormatRegBase * p_reg = static_cast<FileFormatRegBase *>(SLS.GetGlobalObject(GloBaseIdx));
		if(p_reg)
			ok = p_reg->Register(id, mimeType, pMimeSubtype, pExt, pSign);
	}
	return ok;
}

/*static*/int SFileFormat::IdentifyContentTransferEnc(const char * pCte)
{
	int    cte = cteUndef;
	if(!isempty(pCte)) {
		if(sstreqi_ascii(pCte, "base64"))
			cte = cteBase64;
		else if(sstreqi_ascii(pCte, "quoted-printable"))
			cte = cteQuotedPrintable;
		else if(sstreqi_ascii(pCte, "8bit"))
			cte = cte8bit;
		else if(sstreqi_ascii(pCte, "7bit"))
			cte = cte7bit;
		else if(sstreqi_ascii(pCte, "binary"))
			cte = cteBinary;
	}
	return cte;
}

/*static*/int SFileFormat::GetContentTransferEncName(int cte, SString & rBuf)
{
	int    ok = 1;
	switch(cte) {
		case cteBase64: rBuf = "base64"; break;
		case cteQuotedPrintable: rBuf = "quoted-printable"; break;
		case cte8bit: rBuf = "8bit"; break;
		case cte7bit: rBuf = "7bit"; break;
		case cteBinary: rBuf = "binary"; break;
		default: rBuf.Z(); ok = 0; break;
	}
	return ok;
}

static const SIntToSymbTabEntry MimeTypeNameList[] = {
	{ SFileFormat::mtMultipart, "multipart" },
	{ SFileFormat::mtText, "text" },
	{ SFileFormat::mtApplication, "application" },
	{ SFileFormat::mtImage, "image" },
	{ SFileFormat::mtMessage, "message" },
	{ SFileFormat::mtVideo, "video" },
	{ SFileFormat::mtAudio, "audio" },
	{ SFileFormat::mtFont, "font" },
	{ SFileFormat::mtModel, "model" },
	{ SFileFormat::mtExample, "example" }
};

/*static*/int SFileFormat::IdentifyMimeType(const char * pMimeType)
	{ return SIntToSymbTab_GetId(MimeTypeNameList, SIZEOFARRAY(MimeTypeNameList), pMimeType); }
/*static*/int SFileFormat::GetMimeTypeName(int mimeType, SString & rBuf)
	{ return SIntToSymbTab_GetSymb(MimeTypeNameList, SIZEOFARRAY(MimeTypeNameList), mimeType, rBuf); }

/*static*/int SFileFormat::GetMime(int id, SString & rMime)
{
	rMime.Z();
	int    ok = 0;
	if(id == Unkn) { // @v9.7.12
		rMime = "application/octet-stream";
		ok = 2; // @v9.8.12 -1-->2
	}
	else if(GloBaseIdx) {
		const FileFormatRegBase * p_reg = static_cast<const FileFormatRegBase *>(SLS.GetGlobalObject(GloBaseIdx));
		if(p_reg)
			ok = p_reg->GetMime(id, rMime);
	}
	return ok;
}

/*static*/int SFileFormat::GetExt(int id, SString & rExt)
{
	rExt.Z();
	int    ok = 0;
	if(GloBaseIdx) {
		const FileFormatRegBase * p_reg = static_cast<const FileFormatRegBase *>(SLS.GetGlobalObject(GloBaseIdx));
		if(p_reg)
			ok = p_reg->GetExt(id, rExt);
	}
	return ok;
}

SFileFormat::SFileFormat() : Id(0)
{
}

SFileFormat::SFileFormat(int f) : Id(f)
{
}

SFileFormat::operator int () const { return Id; }
int SFileFormat::operator !() const { return (Id == 0); }

int SFileFormat::Identify(const char * pFileName, SString * pExt)
{
	int    ok = 0;
	Id = Unkn;
	if(!isempty(pFileName) && GloBaseIdx) {
		const FileFormatRegBase * p_reg = static_cast<const FileFormatRegBase *>(SLS.GetGlobalObject(GloBaseIdx));
		if(p_reg)
			ok = p_reg->Identify(pFileName, &Id, pExt);
	}
	return ok;
}

int SFileFormat::IdentifyMime(const char * pMime)
{
	int    ok = 0;
	if(GloBaseIdx) {
		const FileFormatRegBase * p_reg = static_cast<const FileFormatRegBase *>(SLS.GetGlobalObject(GloBaseIdx));
		if(p_reg)
			ok = p_reg->IdentifyMime(pMime, &Id);
	}
	return ok;
}

/*static*/int SFileFormat::Register()
{
	int    ok = 1;
	Register(Txt,    mtText,  "plain", "txt;csv", 0);
	Register(Jpeg,   mtImage, "jpeg", "jpg;jpeg;jp2;jfif", "FFD8FFE0");        // JPG
	Register(Jpeg,   "jpg", "FFD8FFE1");                      // JPG
	Register(Jpeg,   "jpg", "0000000C6A5020200D0A870A");      // JP2
	Register(Jpeg,   "jpg", "4A4946393961");                  // JIF
	Register(Jpeg,   "jpg", "FFD8FFE14ED84578696600004949");  // JPG Kodak
	Register(Png,    mtImage, "png",  "png", "89504E470D0A1A0A");   // PNG
	Register(Png,    "png", "889A0D12");                      // PNG
	Register(Tiff,   mtImage, "tiff", "tff;tiff", "49492A00");      // TIFF
	Register(Tiff,   "tff;tiff", "4D4D2A");                   // TIFF
	Register(Gif,    mtImage, "gif", "gif", "47494638");            // GIF
	Register(Bmp,    mtImage, "bmp", "bmp", "424D");                // BMP
	Register(Ico,    mtImage, "x-icon", "ico", "0001");             // ICO
	Register(Cur,    mtApplication, "octet-stream", "cur", "0002"); // CUR
	Register(Xml,    mtApplication, "xml", "xml", "T<?xml");        // XML
	Register(Xsd,    mtApplication, "xml", "xsd", "T<?xml");        // XSD @v10.8.0
	Register(Svg,    mtImage, "svg+xml",   "svg", "T<?xml"); // SVG // @todo Необходимо проверить XML-контент на наличие тега <svg>
	Register(Html,   mtText,  "html",  "html;htm", "T<!DOCTYPE HTML"); // HTML
	Register(Ini,    mtText,  "plain", "ini", static_cast<const char *>(0));  // INI
	Register(Csv,    mtText,  "csv",   "csv", 0); // @v10.8.0
	Register(Tsv,    mtText,  "tsv",   "tsv", 0); // @v10.9.9 tab-separated-values
	Register(Latex,            mtApplication, "x-latex", "tex", static_cast<const char *>(0));  // LATEX
	Register(Latex,            "latex", static_cast<const char *>(0));         // LATEX
	Register(TxtBomUTF8,       "txt;csv", "EFBBBF");
	Register(TxtBomUTF16BE,    "txt;csv", "FEFF");
	Register(TxtBomUTF16LE,    "txt;csv", "FFFE");
	Register(TxtBomUTF32BE,    "txt;csv", "0000FEFF");
	Register(TxtBomUTF32LE,    "txt;csv", "FFFE0000");
	Register(TxtBomUTF7,       "txt;csv", "2B2F7638");
	Register(TxtBomUTF7,       "txt;csv", "2B2F7639");
	Register(TxtBomUTF7,       "txt;csv", "2B2F762B");
	Register(TxtBomUTF7,       "txt;csv", "2B2F762F");
	Register(TxtBomUTF1,       "txt;csv", "F7644C");
	Register(TxtBomUTF_EBCDIC, "txt;csv", "DD736673");
	Register(TxtBomSCSU,       "txt;csv", "OEFEFF");
	Register(TxtBomBOCU1,      "txt;csv", "FBEE28");
	Register(TxtBomGB18030,    "txt;csv", "84319533");
	Register(Pdf, mtApplication, "pdf", "pdf", "25504446");
	Register(Rtf,        "rtf",     "7B5C72746631");
	Register(Mdb,        "mdb",     "000100005374616E64617264204A6574204442");
	Register(AccDb,      "accdb",   "000100005374616E6461726420414345204442");
	Register(WbXml,      "wbxml",   "030B6A");
	Register(Wmf,        "wmf",     "D7CDC69A");
	Register(Eps,        "eps",     "252150532D41646F6265");
	Register(Eps,        "eps",     "C5D0D3C6");
	Register(Hlp,        "hlp",     "6:0000FFFFFFFF");
	Register(Ppd,        "ppd",     "2A5050442D41646F62653A");
	Register(PList,      "plist",   "62706C697374");
	Register(Mat,        "mat",     "4D41544C4142");
	Register(Pdb,        "pdb",     "4D6963726F736F667420432F432B2B20");
	Register(WcbffOld,   "",        "0E11fC0DD0CF11E0");
	Register(Zip,    mtApplication, "zip",    "zip", "504B0304");
	Register(Zip,        "zip", "504B0506");
	Register(Zip,        "zip", "504B0708");
	Register(Rar,        "rar", "52617221");
	Register(Gz,     mtApplication, "x-gzip",  "gz",  "1F8B08");
	Register(Bz2,    mtApplication, "x-bzip2", "bz2", "425A68");
	Register(SevenZ, mtApplication, "x-lzma",  "7z",  "377ABCAF");
	Register(Xz,     mtApplication, "x-xz",    "xz",  "FD377A585A");
	Register(Z,          "z",   "1F9D90");
	Register(Cab,        "cab", "49536328");
	Register(Cab,        "cab", "4D534346");
	Register(Arj,        "arj", "60EA");
	Register(Lzh,        "lzh", "2D6C68");
	Register(Xar,        "xar", "78617221001C");
	Register(Pmd,        "pmd", "8FAFAC84");
	Register(Deb,        "deb", "213C617263683E");
	Register(Rpm,        "rpm", "EDABEEDB");
	Register(Chm,        "chm", "49545346");
	Register(Vhd,        "vhd", "636F6E6563746978");
	Register(Wim,        "wim", "4D5357494D");
	Register(Mdf,        "mdf", "00FFFFFFFFFFFFFFFFFFFF0000020001");
	Register(Nri,        "nri", "0E4E65726F49534F");
	Register(Swf,        "swf", "435753");
	Register(Swf,        "swf", "465753");
	Register(Mar,        "mar", "4D41723000");
	Register(Mar,        "mar", "4D415243");
	Register(Mar,        "mar", "4D41523100");
	Register(Tar,        "tar", "257:7573746172"/*ustar*/); // начиная с 257 байта
	//Register(Iso,  "iso", "32769:4344303031"); // начиная с 32769 байта (пока не поддерживаем)
	Register(Mkv,        "mkv", "1A45DFA3934282886D6174726F736B61");
	Register(Avi,        "avi", "52494646 8:415649204C495354");
	Register(Mp4,        "mp4", "0000002066747970");
	Register(Mp4,        "mp4", "0000001C66747970");
	Register(Wmv,        "wmv", "3026B2758E66CF11");
	Register(Mpg,        "mpg", "000001BA");
	Register(Flv,        "flv", "464C5601");
	Register(Mov,        "mov", "4:6D6F6F76");
	Register(F4f,        "f4f", "4:61667261");
	Register(Class,      "class", "CAFEBABE");        // binary:class 0:CAFEBABE
	Register(Exe,   mtApplication, "octet-stream", "exe",   "4D5A"); // binary:exe   0:4D5A
	Register(Dll,   mtApplication, "x-msdownload", "dll",   "4D5A"); // binary:dll   0:4D5A
	Register(Pcap,       "pcap",  "D4C3B2A1");        // binary:pcap  0:D4C3B2A1
	Register(Pyo,        "pyo",   "03F30D0A");        // binary:pyo   0:03F30D0A
	Register(So,         "so",    "7F454C46");        // binary:so    0:7F454C46
	Register(Mo,         "mo",    "DE120495");        // binary:mo    0:DE120495
	Register(Mui,        "mui",   "50413330");        // binary:mui   0:50413330
	Register(Cat,        "cat",   "0:30 6:2A864886"); // binary:cat   0:30 6:2A864886
	Register(Xsb,        "xsb",   "DA7ABABE");        // binary:xsb   0:DA7ABABE
	Register(Key,        "key",   "4B4C737727");      // binary:key   0:4B4C737727
	Register(Sq3,        "sq3",   "53514C697465");    // binary:sq3   0:53514C697465
	Register(Qst,        "qst",   "0401C4030000");    // binary:qst   0:0401C4030000 binary:qst   0:040180040000
	Register(Qst,        "qst",   "040180040000");    // binary:qst   0:0401C4030000 binary:qst   0:040180040000
	Register(Crx,        "crx",   "43723234");        // binary:crx   0:43723234
	Register(Utx,        "utx",   "4C0069006E006500610067006500"); // binary:utx   0:4C0069006E006500610067006500
	Register(Rx3,        "rx3",   "52583362");         // binary:rx3   0:52583362
	Register(Kdc,        "kdc",   "44494646");         // binary:kdc   0:44494646
	Register(Xnb,        "xnb",   "584E42");           // binary:xnb   0:584E42
	Register(Blp,        "blp",   "424C5031");         // binary:blp   0:424C5031 binary:blp   0:424C5032
	Register(Blp,        "blp",   "424C5032");         // binary:blp   0:424C5031 binary:blp   0:424C5032
	Register(Big,        "big",   "42494746");         // binary:big   0:42494746
	Register(Mdl,        "mdl",   "49445354");         // binary:mdl   0:49445354
	Register(Spr,        "spr",   "CDCC8C3F");         // binary:spr   0:CDCC8C3F
	Register(Sfo,        "sfo",   "00505346");         // binary:sfo   0:00505346
	Register(Mpq,        "mpq",   "4D50511A");         // binary:mpq   0:4D50511A
	Register(Nes,        "nes",   "4E45531A");         // binary:nes   0:4E45531A
	Register(Dmp,        "dmp",   "4D444D5093A7");     // binary:dmp   0:4D444D5093A7
	Register(Dex,        "dex",   "6465780a30333500"); // binary:dex   0:6465780a30333500 binary:dex   0:6465780a30333600
	Register(Dex,        "dex",   "6465780a30333600"); // binary:dex   0:6465780a30333500 binary:dex   0:6465780a30333600
	Register(Gim,        "gim",   "4D49472E30302E31505350"); // binary:gim   0:4D49472E30302E31505350
	Register(Amxx,       "amxx",  "58584D41");               // binary:amxx  0:58584D41
	Register(Sln,        "sln",  "TMicrosoft Visual Studio Solution File"); //
	Register(VCProj,     mtApplication, "xml", "vcproj", "T<?xml");         // 
	Register(VCProj,     "vcxproj", "T<?xml");
	Register(VCProjFilers, mtApplication, "xml", "vcxproj.filters", "T<?xml"); // @v10.9.9 Visual Studio Project Filters
	Register(VCProjUser,   mtApplication, "xml", "vcxproj.user",    "T<?xml"); // @v10.9.9 Visual Studio Project User
	Register(Asm,        "asm", 0);
	Register(C,          "c",   0);
	Register(C,          "c",   "T/*");
	Register(CPP,        "cpp;cxx;cc", 0);
	Register(CPP,        "cpp;cxx;cc", "T/*");
	Register(CPP,        "cpp;cxx;cc", "T//");
	Register(H,          "h;hpp", 0);
	Register(H,          "h;hpp", "T/*");
	Register(H,          "h;hpp", "T//");
	Register(Perl,       "pl",   "T#!perl");
	Register(Perl,       "pm",   "Tpackage");
	Register(Php,        "php",  "T<?php");
	Register(Java,       "java", 0);
	Register(Java,       "java", "Tpackage");
	Register(Java,       "java", "Timport");
	Register(Java,       "java", "T/*");
	Register(Py,         "py", 0);
	Register(UnixShell,  "sh",   "T#!/bin/sh");
	Register(Msi,        mtApplication, "x-ole-storage", "msi",  "D0CF11E0A1B11AE1");
	Register(Log,        "log", 0);
	Register(Properties, "properties", 0);
	Register(Css,        "css", 0);
	Register(JavaScript, "js",  0);
	Register(Json,       mtApplication, "json", "json", "T{");
	Register(Json,       "json", "T[");
	Register(Pbxproj,    "pbxproj", 0);
	Register(Gravity,    "gravity", 0); // @v10.8.2
	Register(PapyruDbDivXchg, mtApplication, "x-papyrus", "pps", "50504F53");
	Register(CodeBlocks_Cbp,  mtApplication, "xml",   "cbp", "T<?xml"); // @v10.9.9 Code::Blocks Project File
	Register(M4,              mtText,        "plain", "m4", 0); // @v10.9.9 m4 macroporcessor
	Register(Webp,            mtImage,       "webp", "webp", "52494646 8:57454250"); // @v11.3.4 webp graphics
	return ok;
}
//
//
//
SEncodingFormat::SEncodingFormat(int f) : Id(oneof2(f, Unkn, Base64) ? f : 0)
{
}

SEncodingFormat::operator int () const
{
	return Id;
}
//
// Descr: Low level data alphabet analyzer
//
SLldAlphabetAnalyzer::Entry::Entry()
{
	Clear();
}

void SLldAlphabetAnalyzer::Entry::Clear()
{
	C = 0;
	LastP = 0;
	P1 = 0.0;
	P2 = 0.0;
}

SLldAlphabetAnalyzer::SLldAlphabetAnalyzer() : Count(0), Status(0)
{
	for(uint i = 0; i < 256; i++) {
		Entry entry;
		Alphabet.insert(&entry);
	}
}

SLldAlphabetAnalyzer::~SLldAlphabetAnalyzer()
{
}

void SLldAlphabetAnalyzer::Clear()
{
	assert(Alphabet.getCount() == 256);
    Status = 0;
    Count = 0;
	for(uint i = 0; i < 256; i++) {
		Alphabet.at(i).Clear();
	}
}

uint64 SLldAlphabetAnalyzer::GetCount() const
{
	return Count;
}

double FASTCALL SLldAlphabetAnalyzer::GetFreq(uint8 s) const
{
	assert(Alphabet.getCount() == 256);
	return fdivnz((double)Alphabet.at(s).C, (double)Count);
}

int FASTCALL SLldAlphabetAnalyzer::GetFreqListOrdered(RAssocArray & rList) const
{
	int    ok = 1;
	assert(Alphabet.getCount() == 256);
	rList.clear();
    for(uint i = 0; i < 256; i++) {
		const Entry & r_entry = Alphabet.at(i);
        const double freq = fdivnz((double)r_entry.C, (double)Count);
        assert(freq >= 0.0);
        if(freq > 0.0) {
			THROW(rList.Add((long)i, freq, 0, 0));
        }
    }
	rList.SortByValRev();
    CATCHZOK
    return ok;
}

double FASTCALL SLldAlphabetAnalyzer::GetPeriodExp(uint8 s) const
{
	assert(Alphabet.getCount() == 256);
	const Entry & r_entry = Alphabet.at(s);
	return fdivnz((double)r_entry.P1, (double)r_entry.C);
}

double FASTCALL SLldAlphabetAnalyzer::GetPeriodStdDev(uint8 s) const
{
	double result = 0.0;
	assert(Alphabet.getCount() == 256);
	const Entry & r_entry = Alphabet.at(s);
	if(r_entry.C > 1) {
		//return (Count > 1) ? (Var * ((double)Count) / ((double)(Count - 1))) : 0.0;
		const double c = (double)r_entry.C;
		const double exp = r_entry.P1 / c;
		const double var = r_entry.P2 / c - exp * (2.0 * r_entry.P1 / c - exp);
		result = sqrt(var * (c / (c-1.0)));
	}
	return result;
}

int FASTCALL SLldAlphabetAnalyzer::AddSymb(uint8 s)
{
	int   ok = 1;
    assert(Alphabet.getCount() == 256);
    Entry & r_entry = Alphabet.at(s);
    Count++;
    r_entry.C++;
    const double delta = (double)(Count - r_entry.LastP);
    r_entry.P1 += delta;
    r_entry.P2 += delta * delta;
    r_entry.LastP = Count;
    return ok;
}

int SLldAlphabetAnalyzer::CollectFileData(const char * pFileName)
{
	int    ok = 1;
    SFile  f_in(pFileName, SFile::mRead|SFile::mBinary|SFile::mNoStd);
    THROW(f_in.IsValid());
    {
        uint8  buffer[8*1024];
        size_t actual_rd_size;
        const uint64 preserve_count = GetCount();
        uint64 count = 0;
        do {
			THROW(f_in.Read(buffer, sizeof(buffer), &actual_rd_size));
			for(uint i = 0; i < actual_rd_size; i++) {
                AddSymb(buffer[i]);
                count++;
			}
        } while(actual_rd_size == sizeof(buffer));
        assert(GetCount() == (count - preserve_count));
    }
    CATCHZOK
	return ok;
}
//
//
//
#if SLTEST_RUNNING // {

SLTEST_R(SFile)
{
	int    ok = 1;
	SFile file;
	SString file_name;
	file_name = MakeOutputFilePath("open_for_write_test.txt");
	//
	// Тестирование функции IsOpenedForWriting()
	//
	// Откроем файл на запись (IsOpenedForWriting должен вернуть 1)
	//
	THROW(SLTEST_CHECK_NZ(file.Open(file_name, SFile::mWrite)));
	SLTEST_CHECK_NZ(SFile::IsOpenedForWriting(file_name));
	SLTEST_CHECK_NZ(file.WriteLine("test"));
	SLTEST_CHECK_NZ(file.Close());
	//
	// Откроем файл на чтение (IsOpenedForWriting должен вернуть 0)
	//
	THROW(SLTEST_CHECK_NZ(file.Open(file_name, SFile::mRead)));
	SLTEST_CHECK_Z(SFile::IsOpenedForWriting(file_name));
	SLTEST_CHECK_NZ(file.Close());
	//
	{
		SLTEST_CHECK_LT(SFile::WaitForWriteSharingRelease(file_name, 10000), 0L);
		THROW(SLTEST_CHECK_NZ(file.Open(file_name, SFile::mWrite)));
		SLTEST_CHECK_Z(SFile::WaitForWriteSharingRelease(file_name, 1000));
		SLTEST_CHECK_NZ(file.Close());
		SLTEST_CHECK_LT(SFile::WaitForWriteSharingRelease(file_name, 1000), 0L);
		//
		// @todo Не проверенным остался случай реального ожидания закрытия файла
		// поскольку для этого надо создавать отдельный асинхронный поток.
		//
	}
	{
		SLTEST_CHECK_NZ(SFile::WildcardMatch("*.*", "abc.txt"));
		SLTEST_CHECK_NZ(SFile::WildcardMatch("*.txt", "abc.txt"));
		SLTEST_CHECK_Z(SFile::WildcardMatch("*.txt", "abc.tx"));
		SLTEST_CHECK_NZ(SFile::WildcardMatch("*xyz*.t?t", "12xyz.txt"));
		SLTEST_CHECK_Z(SFile::WildcardMatch("*xyz*.t?t", "12xyz.txxt"));
		SLTEST_CHECK_NZ(SFile::WildcardMatch("*xyz*.t?t", "12xyz-foo.txt"));
		SLTEST_CHECK_NZ(SFile::WildcardMatch("??xyz*.t?t", "12xyz-foo.txt"));
		SLTEST_CHECK_Z(SFile::WildcardMatch("??xyz*.t?t", "123xyz-foo.txt"));
		SLTEST_CHECK_NZ(SFile::WildcardMatch("??xyz*.x*", "12xyz-foo.xml"));
		SLTEST_CHECK_NZ(SFile::WildcardMatch("??xyz*.x*", "12xyz-foo.xls"));
		SLTEST_CHECK_NZ(SFile::WildcardMatch("??xyz*.x*", "12xyz-foo.xlsm"));
	}
	CATCHZOK;
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
