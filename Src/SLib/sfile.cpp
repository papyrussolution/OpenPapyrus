// SFILE.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024
//
#include <slib-internal.h>
#pragma hdrstop
#include <share.h>
#include <AclAPI.h>
// @v11.7.1 #include <sys/locking.h>

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

SFile::Stat::Stat()
{
	THISZERO();
}

SFile::Stat & SFile::Stat::Z()
{
	THISZERO();
	return *this;
}

bool SFile::Stat::IsFolder() const { return LOGIC(Attr & SFile::attrSubdir); }
bool SFile::Stat::IsSymLink() const { return ((Attr & attrReparsePoint) && ReparsePointTag == IO_REPARSE_TAG_SYMLINK); }

int SFile::Stat::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, Flags, rBuf));
	THROW(pSCtx->Serialize(dir, Attr, rBuf));
	THROW(pSCtx->Serialize(dir, Size, rBuf));
	THROW(pSCtx->Serialize(dir, ReparsePointTag, rBuf));
	THROW(pSCtx->Serialize(dir, CrtTm_, rBuf));
	THROW(pSCtx->Serialize(dir, AccsTm_, rBuf));
	THROW(pSCtx->Serialize(dir, ModTm_, rBuf));
	CATCHZOK
	return ok;
}

/*static*/bool SFile::GetReparsePoint(SIntHandle h, SBinaryChunk & rC)
{
	bool   ok = false;
	rC.Z();
	DWORD size = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
	if(rC.Ensure(size)) {
		if(::DeviceIoControl(h, FSCTL_GET_REPARSE_POINT, NULL, 0, rC.Ptr(), size, &size, NULL)) {
			assert(size <= rC.GetAllocatedSize());
			if(rC.Put(rC.PtrC(), size)) {
				ok = true;
			}
		}
	}
	return ok;
}

/*static*/bool SFile::SetReparsePoint(SIntHandle h, const SBinaryChunk & rC)
{
	bool   ok = false;
	void * p_data = const_cast<void *>(rC.PtrC());
	DWORD size = rC.Len();
	if(::DeviceIoControl(h, FSCTL_SET_REPARSE_POINT, p_data, size, 0, 0, &size, 0)) {
		ok = true;
	}
	return ok;
}

/*static*/bool SFile::DeleteReparsePoint(SIntHandle h, const SBinaryChunk & rC)
{
	bool ok = false;
	if(!!h && rC.Len()) {
		const DWORD _reparse_data_buffer_header_size = FIELD_OFFSET(ReparseDataBuffer, GenericReparseBuffer);
		// /*REPARSE_DATA_BUFFER*/ReparseDataBuffer rdbuf;
		const ReparseDataBuffer * p_rdb_src = static_cast<const ReparseDataBuffer *>(rC.PtrC());
		if(p_rdb_src->ReparseTag & 0xB0000000) { // (0x80000000|0x20000000|0x10000000)
			DWORD size = IsReparseTagMicrosoft(static_cast<const ReparseDataBuffer *>(rC.PtrC())->ReparseTag) ? _reparse_data_buffer_header_size : REPARSE_GUID_DATA_BUFFER_HEADER_SIZE;
			SBinaryChunk rdbuf;
			rdbuf.Put(rC.PtrC(), size);
			ReparseDataBuffer * p_rdb_temp = static_cast<ReparseDataBuffer *>(rdbuf.Ptr());
			//memcpy(&rdbuf, pBuf, size);
			p_rdb_temp->ReparseDataLength = 0;
			ok = ::DeviceIoControl(h, FSCTL_DELETE_REPARSE_POINT, p_rdb_temp, size, 0, 0, &size, NULL);
		}
	}
	return ok;
}

#if 0 // @construction {
/*static*/int SFile::SetStatExt(SIntHandle h, uint flags, const Stat * pStat, const SBinarySet * pExtSet) // @v11.8.11
{
	int    ok = 0;
	if(!!h && pStat) {
		FILE_BASIC_INFO fi_basic;
		FILETIME ft;
		fi_basic.FileAttributes = pStat->Attr;
		ldatetime_to_wftime(&pStat->CrtTime, &ft);
		//fi_basic.CreationTime = pStat->CrtTime.Get();
		if(SetFileInformationByHandle(h, FileBasicInfo, &fi_basic, sizeof(fi_basic))) {
		}
	}
	return ok;
}
#endif // } 0
	
/*static*/int SFile::GetStatExt(SIntHandle h, uint flags, Stat * pStat, SBinarySet * pExtSet) // @v11.8.9
{
	int    ok = 0;
	if(!!h) {
		FILE_BASIC_INFO fi_basic;
		FILE_ATTRIBUTE_TAG_INFO fi_attr;
		enum {
			okfFileBasicInfo = 0x0001,
			okfFileAttributeTagInfo = 0x0002,
			okfGetFileSizeEx = 0x0004
		};
		uint okf = 0;
		if(GetFileInformationByHandleEx(h, FileBasicInfo, &fi_basic, sizeof(fi_basic))) {
			okf |= okfFileBasicInfo;
			pStat->CrtTm_ = fi_basic.ChangeTime.QuadPart;
			pStat->AccsTm_ = fi_basic.LastAccessTime.QuadPart;
			pStat->ModTm_ = fi_basic.LastWriteTime.QuadPart;
			pStat->Attr = fi_basic.FileAttributes;
		}
		if(GetFileInformationByHandleEx(h, FileAttributeTagInfo, &fi_attr, sizeof(fi_attr))) {
			okf |= okfFileAttributeTagInfo;
			if(okf & okfFileBasicInfo) {
				assert(fi_attr.FileAttributes == fi_basic.FileAttributes);
			}
			pStat->ReparsePointTag = fi_attr.ReparseTag;
		}
		{
			LARGE_INTEGER size;
			if(GetFileSizeEx(h, &size)) {
				okf |= okfGetFileSizeEx;
				pStat->Size = size.QuadPart;
			}
		}
		/*
			BOOL GetFileInformationByHandleEx([in]  HANDLE hFile, [in]  FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
				[out] LPVOID lpFileInformation, [in]  DWORD dwBufferSize);


			FileBasicInfo
			Minimal information for the file should be retrieved or set. Used for file handles. See
			FILE_BASIC_INFO.

			FileStandardInfo
			Extended information for the file should be retrieved. Used for file handles. Use only when calling
			GetFileInformationByHandleEx. See
			FILE_STANDARD_INFO.

			FileNameInfo
			The file name should be retrieved. Used for any handles. Use only when calling
			GetFileInformationByHandleEx. See
			FILE_NAME_INFO.

			FileRenameInfo
			The file name should be changed. Used for file handles. Use only when calling
			SetFileInformationByHandle. See
			FILE_RENAME_INFO.

			FileDispositionInfo
			The file should be deleted. Used for any handles. Use only when calling
			SetFileInformationByHandle. See
			FILE_DISPOSITION_INFO.

			FileAllocationInfo
			The file allocation information should be changed. Used for file handles. Use only when calling
			SetFileInformationByHandle. See
			FILE ALLOCATION INFO.

			FileEndOfFileInfo
			The end of the file should be set. Use only when calling
			SetFileInformationByHandle. See
			FILE_END_OF_FILE_INFO.

			FileStreamInfo
			File stream information for the specified file should be retrieved. Used for any handles. Use only when calling
			GetFileInformationByHandleEx. See
			FILE_STREAM_INFO.

			FileCompressionInfo
			File compression information should be retrieved. Used for any handles. Use only when calling
			GetFileInformationByHandleEx. See
			FILE_COMPRESSION_INFO.

			FileAttributeTagInfo
			File attribute information should be retrieved. Used for any handles. Use only when calling
			GetFileInformationByHandleEx. See
			FILE_ATTRIBUTE_TAG_INFO.

			FileIdBothDirectoryInfo
			Files in the specified directory should be retrieved. Used for directory handles. Use only when calling
			GetFileInformationByHandleEx. The number
			of files returned for each call to
			GetFileInformationByHandleEx depends on
			the size of the buffer that is passed to the function. Any subsequent calls to
			GetFileInformationByHandleEx on the same
			handle will resume the enumeration operation after the last file is returned. See
			FILE_ID_BOTH_DIR_INFO.

			FileIdBothDirectoryRestartInfo
			Identical to FileIdBothDirectoryInfo, but forces the enumeration operation to
			start again from the beginning. See
			FILE_ID_BOTH_DIR_INFO.

			FileIoPriorityHintInfo
			Priority hint information should be set. Use only when calling
			SetFileInformationByHandle. See
			FILE_IO_PRIORITY_HINT_INFO.

			FileRemoteProtocolInfo
			File remote protocol information should be retrieved. Use for any handles. Use only when calling
			GetFileInformationByHandleEx. See
			FILE_REMOTE_PROTOCOL_INFO.

			FileFullDirectoryInfo
			Files in the specified directory should be retrieved. Used for directory handles. Use only when calling
			GetFileInformationByHandleEx. See
			FILE_FULL_DIR_INFO.
			Windows Server 2008 R2, Windows 7, Windows Server 2008, Windows Vista, Windows Server 2003 and Windows XP:  This value is not supported before Windows 8 and Windows Server 2012

			FileFullDirectoryRestartInfo
			Identical to FileFullDirectoryInfo, but forces the enumeration operation to
			start again from the beginning. Use only when calling
			GetFileInformationByHandleEx. See
			FILE_FULL_DIR_INFO.
			Windows Server 2008 R2, Windows 7, Windows Server 2008, Windows Vista, Windows Server 2003 and Windows XP:  This value is not supported before Windows 8 and Windows Server 2012

			FileStorageInfo
			File storage information should be retrieved. Use for any handles. Use only when calling
			GetFileInformationByHandleEx. See
			FILE_STORAGE_INFO.
			Windows Server 2008 R2, Windows 7, Windows Server 2008, Windows Vista, Windows Server 2003 and Windows XP:  This value is not supported before Windows 8 and Windows Server 2012

			FileAlignmentInfo
			File alignment information should be retrieved. Use for any handles. Use only when calling
			GetFileInformationByHandleEx. See
			FILE_ALIGNMENT_INFO.
			Windows Server 2008 R2, Windows 7, Windows Server 2008, Windows Vista, Windows Server 2003 and Windows XP:  This value is not supported before Windows 8 and Windows Server 2012

			FileIdInfo
			File information should be retrieved. Use for any handles. Use only when calling
			GetFileInformationByHandleEx. See
			FILE_ID_INFO.
			Windows Server 2008 R2, Windows 7, Windows Server 2008, Windows Vista, Windows Server 2003 and Windows XP:  This value is not supported before Windows 8 and Windows Server 2012

			FileIdExtdDirectoryInfo
			Files in the specified directory should be retrieved. Used for directory handles. Use only when calling
			GetFileInformationByHandleEx. See
			FILE_ID_EXTD_DIR_INFO.
			Windows Server 2008 R2, Windows 7, Windows Server 2008, Windows Vista, Windows Server 2003 and Windows XP:  This value is not supported before Windows 8 and Windows Server 2012

			FileIdExtdDirectoryRestartInfo
			Identical to FileIdExtdDirectoryInfo, but forces the enumeration operation to
			start again from the beginning. Use only when calling
			GetFileInformationByHandleEx. See
			FILE_ID_EXTD_DIR_INFO.
			Windows Server 2008 R2, Windows 7, Windows Server 2008, Windows Vista, Windows Server 2003 and Windows XP:  This value is not supported before Windows 8 and Windows Server 2012

			FileDispositionInfoEx
			FileRenameInfoEx
			MaximumFileInfoByHandleClass
			This value is used for validation. Supported values are less than this value.
		*/
	}
	return ok;
}

/*static*/int SFile::GetStat(const char * pFileName, uint flags, Stat * pStat, SBinarySet * pExtSet)
{
	EXCEPTVAR(SLibError);
	int    ok = 1; // @v11.2.0 @fix (-1)-->(1)
#ifdef __WIN32__
	{
		SDirEntry de;
		if(SDirec::GetSingle(pFileName, &de)) {
			if(de.Attr & SFile::attrSubdir) {
			}
			else {
			}
			if(de.Attr & SFile::attrReparsePoint) {
				if(pExtSet) {
					if(oneof2(de.ReparsePointTag, IO_REPARSE_TAG_SYMLINK, IO_REPARSE_TAG_MOUNT_POINT)) {
						SBinaryChunk rp;
						SString _file_name(pFileName);
						SStringU _file_name_u;
						_file_name.CopyToUnicode(_file_name_u);
						SIntHandle h = ::CreateFileW(_file_name_u, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT, NULL);
						if(!!h) {
							if(SFile::GetReparsePoint(h, rp)) {
								const ReparseDataBuffer * p_rdb = static_cast<const ReparseDataBuffer *>(rp.PtrC());
								pExtSet->Put(Stat::sbiRaparseTag, rp);
							}
							::CloseHandle(h);
						}
					}
				}
			}
			ASSIGN_PTR(pStat, *static_cast<const Stat *>(&de));
		}
		else {
			ok = 0;
		}
		/* @v11.8.9 @mustbe
		WIN32_FILE_ATTRIBUTE_DATA fa;
		if(::GetFileAttributesEx(_file_name_u, GetFileExInfoStandard, &fa)) {
			//
		}
		or
		GetFileInformationByHandleEx
		*/
#if 0 // {
		h_file = ::CreateFileW(_file_name_u, FILE_READ_ATTRIBUTES|FILE_READ_EA/*|STANDARD_RIGHTS_READ*/, 
			0/*FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE*/, 0, OPEN_EXISTING, 
			FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_DIRECTORY|FILE_FLAG_OPEN_REPARSE_POINT, 0); 
		SLS.SetAddedMsgString(pFileName);
		THROW_V(h_file, SLERR_OPENFAULT);
		THROW(GetStat(h_file, flags, pStat, pExtSet));
		//SFile::GetTime(h_file, &stat.CrtTime, &stat.AccsTime, &stat.ModTime);
		//GetFileSizeEx(h_file, &size);
		//stat.Size = size.QuadPart;
#endif // } 0
	}
#else // non-windows os
	// @todo
#endif
	return ok;
}

/*static*/int SFile::GetDiskSpace(const char * pPath, int64 * pTotal, int64 * pAvail)
{
	int    ok = 1;
	ULARGE_INTEGER avail, total, total_free;
	SString path;
	SFsPath ps(pPath);
	ps.Merge(0, SFsPath::fNam|SFsPath::fExt, path);
	if(GetDiskFreeSpaceEx(SUcSwitch(path), &avail, &total, &total_free)) {
		ASSIGN_PTR(pTotal, total.QuadPart);
		ASSIGN_PTR(pAvail, avail.QuadPart);
	}
	else {
		ASSIGN_PTR(pTotal, 0);
		ASSIGN_PTR(pAvail, 0);
		ok = SLS.SetOsError();
	}
	return ok;
}

/*static*/int SFile::GetSecurity(const char * pFileNameUtf8, uint requestFlags, SBuffer & rSecurInfo)
{
	rSecurInfo.Z();
	int    ok = 0;
	/*
		requestFlags:
		#define OWNER_SECURITY_INFORMATION                  (0x00000001L)
		#define GROUP_SECURITY_INFORMATION                  (0x00000002L)
		#define DACL_SECURITY_INFORMATION                   (0x00000004L)
		#define SACL_SECURITY_INFORMATION                   (0x00000008L)
		#define LABEL_SECURITY_INFORMATION                  (0x00000010L)
		#define ATTRIBUTE_SECURITY_INFORMATION              (0x00000020L)
		#define SCOPE_SECURITY_INFORMATION                  (0x00000040L)
		#define PROCESS_TRUST_LABEL_SECURITY_INFORMATION    (0x00000080L)
		#define ACCESS_FILTER_SECURITY_INFORMATION          (0x00000100L)
		#define BACKUP_SECURITY_INFORMATION                 (0x00010000L)

		#define PROTECTED_DACL_SECURITY_INFORMATION         (0x80000000L)
		#define PROTECTED_SACL_SECURITY_INFORMATION         (0x40000000L)
		#define UNPROTECTED_DACL_SECURITY_INFORMATION       (0x20000000L)
		#define UNPROTECTED_SACL_SECURITY_INFORMATION       (0x10000000L) 

		errors:
			ERROR_PRIVILEGE_NOT_HELD - 1314 (0x522) - Клиент не располагает требуемыми правами доступа.
	*/
	if(!isempty(pFileNameUtf8)) {
		SECURITY_DESCRIPTOR * p_secur_descr = 0;
		STempBuffer secur_descr_buf(512);
		SString & r_temp_buf = SLS.AcquireRvlStr();
		SStringU & r_temp_buf_u = SLS.AcquireRvlStrU();
		DWORD  secur_size = 0;
		r_temp_buf = pFileNameUtf8;
		THROW(r_temp_buf.IsLegalUtf8());
		THROW(r_temp_buf_u.CopyFromUtf8(r_temp_buf));
		THROW(secur_descr_buf.IsValid());
		BOOL res = ::GetFileSecurityW(r_temp_buf_u, requestFlags, (PSECURITY_DESCRIPTOR)secur_descr_buf, secur_descr_buf.GetSize(), &secur_size);
		if(res) {
			rSecurInfo.Write(secur_descr_buf.vcptr(), secur_size);
			ok = 1;
		}
		else {
			int last_err = GetLastError();
			if(last_err == ERROR_INSUFFICIENT_BUFFER) {
				THROW(secur_descr_buf.Alloc(secur_size));
				res = ::GetFileSecurityW(r_temp_buf_u, requestFlags, (PSECURITY_DESCRIPTOR)secur_descr_buf, secur_descr_buf.GetSize(), &secur_size);
				if(res) {
					rSecurInfo.Write(secur_descr_buf.vcptr(), secur_size);
					ok = 1;
				}
			}
			else {
			}
		}
	}
	CATCHZOK
	return ok;
}

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

/*static*/int SFile::CreateDirByTemplate(const char * pPath, const char * pTemplate) // @v11.8.12
{
	int    ok = 1;
	SString path;
	SString temp_path;
	SStringU temp_buf_u;
	// @v11.2.12 (temp_path = pPath).SetLastSlash().ReplaceChar('/', '\\');
	// @v11.8.4 SFsPath::npfCompensateDotDot
	SFsPath::NormalizePath(pPath, SFsPath::npfKeepCase|SFsPath::npfCompensateDotDot, temp_path).SetLastSlash(); // @v11.2.12 
	const char * p = temp_path;
	do {
		if(*p == '\\') {
			if(p[1] == '\\')
				path.CatChar(*p++);
			else if(path.NotEmpty()) {
				const bool is_root = (path[0] == path[1] && path[0] == '\\' && !sstrchr(path+2, '\\'));
				path.CopyToUnicode(temp_buf_u);
				if(!is_root && (temp_buf_u[0] && ::_waccess(temp_buf_u, 0) != 0)) {
					const bool is_terminal = !sstrchr(p+1, '\\');
					if(is_terminal && !isempty(pTemplate)) {
						SStringU template_buf_u;
						SString template_buf(pTemplate);
						template_buf.CopyToUnicode(template_buf_u);
						const int cdr = CreateDirectoryExW(template_buf_u, temp_buf_u, 0);
						if(!cdr) {
							SString added_msg_buf;
							added_msg_buf.Cat(template_buf).Cat("->").Cat(path);
							ok = SLS.SetError(SLERR_MKDIRBYTEMPLATEFAULT, added_msg_buf);
						}
						else
							ok = 1;
					}
					else {
						const int cdr = ::CreateDirectoryW(temp_buf_u, NULL);
						ok = (cdr == 0) ? SLS.SetError(SLERR_MKDIRFAULT, path) : 1;
					}
				}
			}
		}
		path.CatChar(*p);
	} while(ok && *p++ != 0);
	return ok;
}

/*static*/int SFile::CreateDir(const char * pPath) // @v11.8.11
{
	return CreateDirByTemplate(pPath, 0);
}

static const SIntToSymbTabEntry SFileAccsfSymbList[] = {
	{ SFile::accsf_DELETE,       "delete" },
	{ SFile::accsf_READCONTROL,  "readcontrol" },
	{ SFile::accsf_WRITE_DAC,    "write_dac" },
	{ SFile::accsf_WRITE_OWNER,  "write_owner" },
	{ SFile::accsf_SYNCHRONIZE,  "synchronize" },
	{ SFile::accsf_STD_REQUIRED, "std_required" },
	{ SFile::accsf_STD_READ,     "std_read" },
	{ SFile::accsf_STD_WRITE,    "std_write" },
	{ SFile::accsf_STD_EXEC,     "std_exec" },
	{ SFile::accsf_STD_ALL,      "std_all" },
	{ SFile::accsf_SPECIFIC_ALL, "specific_all" },
	{ SFile::accsfDataRead,      "dataread" },
	{ SFile::accsfDataWrite,     "datawrite" },
	{ SFile::accsfDataAppend,    "dataappend" },
	{ SFile::accsfDirList,       "dirlist" },
	{ SFile::accsfDirAddFile,    "diraddfile" },
	{ SFile::accsfDirAddSub,     "diraddsub" },
	{ SFile::accsfPipeCreate,    "pipecreate" },
	{ SFile::accsfEaRead,        "earead" },
	{ SFile::accsfEaWrite,       "eawrite" },
	{ SFile::accsfExec,          "exec" },
	{ SFile::accsfDirTraverse,   "dirtraverse" },
	{ SFile::accsfDirDelete,     "dirdelete" },
	{ SFile::accsfAttrRead,      "attrread" },
	{ SFile::accsfAttrWrite,     "attrwrite" },
	{ SFile::accsfAll,           "all" },
	{ SFile::accsfGenericRead,   "genericread" },
	{ SFile::accsfGenericWrite,  "genericwrite" },
	{ SFile::accsfGenericExec,   "genericexec" },
};

/*static*/uint SFile::ParseAccsf(const char * pSymb) // @v11.8.2 @construction
{
	uint   accsf = 0;
	if(!isempty(pSymb)) {
		SString temp_buf;
		SStrScan scan(pSymb);
		while(scan.GetIdent(temp_buf)) {
			int f = SIntToSymbTab_GetId(SFileAccsfSymbList, SIZEOFARRAY(SFileAccsfSymbList), temp_buf);
			if(f) {
				accsf |= static_cast<uint>(f);
				scan.SkipOptionalDiv('|', SStrScan::wsSpace|SStrScan::wsTab);
			}
			else {
				accsf = 0; // @error
				break;
			}
		}
	}
	return accsf;
}

/*static*/int SFile::GetAccsfSymb(uint accsf, SString & rBuf) // @v11.8.2 @construction
{
	rBuf.Z();
	int    ok = 0;
	if(accsf) {
		if(oneof4(accsf, accsfAll, accsfGenericRead, accsfGenericWrite, accsfGenericExec))
			ok = SIntToSymbTab_GetSymb(SFileAccsfSymbList, SIZEOFARRAY(SFileAccsfSymbList), accsf, rBuf);
		/*else if(SBits::Cpop(accsf) == 1) {
			ok = SIntToSymbTab_GetSymb(SFileAccsfSymbList, SIZEOFARRAY(SFileAccsfSymbList), accsf, rBuf);
		}
		else {
			//for()
		}*/
		//accsfAll                  = (accsf_STD_REQUIRED|accsf_SYNCHRONIZE|0x1ff),
		//accsfGenericRead          = (accsf_STD_READ|accsfDataRead|accsfAttrRead|accsfEaRead|accsf_SYNCHRONIZE),
		//accsfGenericWrite         = (accsf_STD_WRITE|accsfDataWrite|accsfAttrWrite|accsfEaWrite|accsfDataAppend|accsf_SYNCHRONIZE),
		//accsfGenericExec          = (accsf_STD_EXEC|accsfAttrRead|accsfExec|accsf_SYNCHRONIZE))
	}
	return ok;
}

/*static*/int FASTCALL SFile::Remove(const char * pFileName)
{
	// @v11.8.12 return isempty(pFileName) ? -1 : ((::remove(pFileName) == 0) ? 1 : SLS.SetError(SLERR_FILE_DELETE, pFileName));
	// @v11.8.12 {
	int    ok = -1;
	if(isempty(pFileName)) {
		ok = -1;
	}
	else {
#ifdef __WIN32__
		SFile::Stat stat;
		if(SFile::GetStat(pFileName, 0, &stat, 0)) {
			if(stat.Attr & SFile::attrSubdir) {
				ok = RemoveDir(pFileName);
			}
			else {
				SStringU & r_temp_buf_u = SLS.AcquireRvlStrU();
				(SLS.AcquireRvlStr() = pFileName).Strip().CopyToUnicode(r_temp_buf_u);
				ok = DeleteFileW(r_temp_buf_u);
			}
		}
		else
			ok = 0;
#else
		ok = ::remove(pFileName);
#endif
		if(!ok)
			SLS.SetError(SLERR_FILE_DELETE, pFileName);
	}
	// } @v11.8.12
	return ok;
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

/*static*/int SFile::SetTime(SIntHandle hFile, int64 tmNs100Creation, int64 tmNs100LastAccess, int64 tmNs100LastModif)
{
#ifdef __WIN32__
	FILETIME w_cr_ft;
	FILETIME * p_w_cr_ft = 0;
	FILETIME w_la_ft;
	FILETIME * p_w_la_ft = 0;
	FILETIME w_lm_ft;
	FILETIME * p_w_lm_ft = 0;
	if(tmNs100Creation) {
		p_w_cr_ft = &w_cr_ft;
		w_cr_ft = *reinterpret_cast<const FILETIME *>(&tmNs100Creation); // @v11.9.1 @fix (tmNs100Creation)-->(&tmNs100Creation)
	}
	if(tmNs100LastAccess) {
		p_w_la_ft = &w_la_ft;
		w_la_ft = *reinterpret_cast<const FILETIME *>(&tmNs100LastAccess); // @v11.9.1 @fix (tmNs100LastAccess)-->(&tmNs100LastAccess)
	}
	if(tmNs100LastModif) {
		p_w_lm_ft = &w_lm_ft;
		w_lm_ft = *reinterpret_cast<const FILETIME *>(&tmNs100LastModif); // @v11.9.1 @fix (tmNs100LastModif)-->(&tmNs100LastModif)
	}
	if(p_w_cr_ft || p_w_la_ft || p_w_lm_ft)
		return BIN(::SetFileTime(hFile, p_w_cr_ft, p_w_la_ft, p_w_lm_ft));
	else
		return 0;
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
	return BIN(setftime(SIntHandle hFile, &ftm) == 0);
#endif
}

/*static*/int SFile::SetTime(SIntHandle hFile, const LDATETIME * pCreation, const LDATETIME * pLastAccess, const LDATETIME * pLastModif)
{
#ifdef __WIN32__
	FILETIME w_cr_ft;
	FILETIME * p_w_cr_ft = 0;
	FILETIME w_la_ft;
	FILETIME * p_w_la_ft = 0;
	FILETIME w_lm_ft;
	FILETIME * p_w_lm_ft = 0;
	if(pCreation) {
		p_w_cr_ft = &w_cr_ft;
		ldatetime_to_wftime(pCreation, p_w_cr_ft);
	}
	if(pLastAccess) {
		p_w_la_ft = &w_la_ft;
		ldatetime_to_wftime(pLastAccess, p_w_la_ft);
	}
	if(pLastModif) {
		p_w_lm_ft = &w_lm_ft;
		ldatetime_to_wftime(pLastModif,  p_w_lm_ft);
	}
	if(p_w_cr_ft || p_w_la_ft || p_w_lm_ft)
		return BIN(::SetFileTime(hFile, p_w_cr_ft, p_w_la_ft, p_w_lm_ft));
	else
		return 0;
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
	return BIN(setftime(SIntHandle hFile, &ftm) == 0);
#endif
}

/*static*/int SFile::GetTime(const char * pFileName, LDATETIME * pCreation, LDATETIME * pLastAccess, LDATETIME * pLastModif)
{
	int    ok = 1;
	SIntHandle h_file = ::CreateFile(SUcSwitch(pFileName), FILE_READ_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0);
	if(h_file) {
		ok = SFile::GetTime(h_file, pCreation, pLastAccess, pLastModif);
		::CloseHandle(h_file);
	}
	else
		ok = 0;
	return ok;
}

/*static*/int SFile::GetTime(SIntHandle hFile, LDATETIME * creation, LDATETIME * lastAccess, LDATETIME * lastModif)
{
#ifdef __WIN32__
	FILETIME w_cr_ft;
	FILETIME w_la_ft;
	FILETIME w_lm_ft;
	if(::GetFileTime(hFile, &w_cr_ft, &w_la_ft, &w_lm_ft)) {
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
	if(getftime(hFile, &ftm) == 0) {
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
	if(!r)
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
	//F = 0;
	H = 0;
	IH = -1;
	Mode = 0;
	Name.Z();
}

int SFile::InvariantC(SInvariantParam * pInvP) const
{
	S_INVARIANT_PROLOG(pInvP);
	S_ASSERT_P(!oneof3(T, tStdFile, tSBuffer, tArchive) || !!H, pInvP);
	S_ASSERT_P(T != tFile   || IH != -1, pInvP);
	//S_ASSERT_P(T != tSBuffer || P_Sb, pInvP);
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

SFile::operator FILE * () { return GetFilePtr(); }
SFile::operator SBuffer * () { return GetSBufPtr(); }
int   SFile::FileNo() const { return GetFilePtr() ? fileno(GetFilePtr()) : ((T == tFile) ? IH : -1); }
const SString & SFile::GetName() const { return Name; }
long  SFile::GetMode() const { return Mode; }

int SFile::GetBuffer(SBaseBuffer & rBuf) const
{
	SBuffer * p_sb = GetSBufPtr();
	if(p_sb) {
		rBuf.P_Buf = (char *)p_sb->GetBuf(0); // @attention @badcast
		rBuf.Size = p_sb->GetAvailableSize();
		return 1;
	}
	else
		return 0;
}

bool SFile::IsValid() const
{
	assert(InvariantC(0));
	bool ok = (GetFilePtr() || GetSBufPtr() || (T == tArchive && !!H) || IH >= 0 || T == tNullOutput);
	if(!ok)
		SLS.SetError(SLERR_FILENOTOPENED);
	return ok;
	//return F ? true : ((IH >= 0 || T == tNullOutput) ? true : SLS.SetError(SLERR_FILENOTOPENED));
}

int SFile::Open(SBuffer & rBuf, long mode)
{
	assert(InvariantC(0));
	int    ok = 1;
	Close();
	{
		H = new SBuffer(rBuf);
		THROW_S(H, SLERR_NOMEM);
		T = tSBuffer;
	}
	{
		const long m = (mode & ~(mBinary | mDenyRead | mDenyWrite | mNoStd));
		Mode = mode;
		if(m == mReadWriteTrunc) {
			assert(GetSBufPtr());
			GetSBufPtr()->SetWrOffs(0);
		}
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
	SString _file_name(pName);
	// @v10.6.0 const  long   m = (mode & ~(mBinary | mDenyRead | mDenyWrite | mNoStd | mNullWrite));
	int    oflag = 0;
	int    pflag = S_IREAD | S_IWRITE;
	int    shflag = 0;
	Mode = mode;
	const  long en_mode = (mode & ~(mBinary|mDenyRead|mDenyWrite|mNoStd|mNullWrite|mBuffRd)); // Перечисляемая часть режима открытия файла
	switch(en_mode) { // @v10.6.0
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
	if(_file_name.HasChr('?')) { // @v11.9.5 Возможно, это - архив
		SString left_buf;
		SString right_buf;
		SString temp_buf;
		const int dr = _file_name.Divide('?', left_buf, right_buf);
		assert(dr > 0); // Не может быть, что dr <= 0 ибо мы выше проверили, что в строке есть '?'
		if(dr > 0) {
			if(left_buf.HasChr('?') || right_buf.HasChr('?')) {
				// @todo @err (недопустимое имя файла)
				ok = 0;
			}
			else {
				if(fileExists(left_buf)) {
					// Трактуем строку pFileName как имя файла внутри архива. При этом
					// на текущий момент left_buf - имя архива, right_buf - имя файла внутри архива
					if(en_mode != mRead) { // Архив может быть открыт объектом SFile только для чтения!
						ok = 0;
					}
					else {
						SFileEntryPool fep;
						int    arc_format = 0;
						right_buf = SFsPath::NormalizePath(right_buf, SFsPath::npfSlash|SFsPath::npfCompensateDotDot, temp_buf);
						if(SArchive::List(SArchive::providerLA, &arc_format, left_buf, 0, fep) > 0) {
							SFileEntryPool::Entry fe;
							SString arc_sub;
							for(uint i = 0; i < fep.GetCount(); i++) {
								if(fep.Get(i, &fe, &arc_sub) > 0) {
									if(SFsPath::NormalizePath(arc_sub, SFsPath::npfSlash|SFsPath::npfCompensateDotDot, temp_buf).IsEqiUtf8(right_buf)) {
										H = SArchive::OpenArchiveEntry(SArchive::providerLA, left_buf, right_buf);
										if(!!H) {
											T = tArchive;
											ok = 1;
										}
										break;
									}
								}
							}
						}
					}
				}
				else {
					ok = 0;
				}
			}
		}
	}
	else { // Валидное имя файла не может содержать символов '?'
		//
		// @v11.6.0 Добавлена предварительная обработка имени файла на предмет кодировки:
		//   - если имя файла состоит только из ascii-символов, то все как обычно (sopen)
		//   - если имя файла состоит из валидных utf8-символов, то преобразуем его в unicode и используем _wsopen
		//   - в противном случае уповаем на провидение и используем sopen как есть.
		//
		if(_file_name.IsAscii()) {
			IH = sopen(_file_name, oflag, shflag, pflag);
		}
		else if(_file_name.IsLegalUtf8()) {
			SStringU & r_temp_buf_u = SLS.AcquireRvlStrU();
			r_temp_buf_u.CopyFromUtf8(_file_name);
			IH = _wsopen(r_temp_buf_u.ucptr(), oflag, shflag, pflag);
		}
		else {
			IH = sopen(pName, oflag, shflag, pflag);
		}
		//_wsopen(
		if(IH >= 0) {
			if(!(mode & mNoStd)) {
				H = fdopen(IH, mode_buf);
				if(!!H)
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
	}
	return ok;
}

int SFile::Close()
{
	assert(InvariantC(0));
	int    ok = 1;
	if(T == tSBuffer) {
		delete GetSBufPtr();
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
		FILE * p_f = GetFilePtr();
		if(p_f) {
			fclose(p_f);
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

bool SFile::Seek(long offs, int origin)
{
	bool   ok = true;
	assert(oneof3(origin, SEEK_SET, SEEK_CUR, SEEK_END));
	assert(offs >= 0);
	assert(InvariantC(0));
	SBuffer * p_sb = GetSBufPtr();
	if(p_sb) {
		long o;
		if(origin == SEEK_CUR)
			o = static_cast<long>(p_sb->GetRdOffs() + offs);
		else if(origin == SEEK_END)
			o = static_cast<long>(p_sb->GetWrOffs() + offs);
		else
			o = offs;
		if(checkirange(o, 0L, static_cast<long>(p_sb->GetWrOffs()))) {
			p_sb->SetRdOffs(o);
			ok = true;
		}
		else
			ok = false;
	}
	else {
		if(T == tArchive) {
			if(!!H) {
				ok = LOGIC(SArchive::SeekArchiveEntry(H, offs, origin));
			}
			else
				ok = false;
		}
		else {
			FILE * p_f = GetFilePtr();
			if(p_f) {
				ok = (fseek(p_f, offs, origin) == 0);
			}
			else if(IH >= 0) {
				ok = (lseek(IH, offs, origin) >= 0);
			}
			else
				ok = false;
		}
	}
	BufR.Z();
	return ok;
}

bool SFile::Seek64(int64 offs, int origin)
{
	bool   ok = true;
	assert(oneof3(origin, SEEK_SET, SEEK_CUR, SEEK_END));
	assert(offs >= 0LL);
	assert(InvariantC(0));
	SBuffer * p_sb = GetSBufPtr();
	if(p_sb) {
		int64  o;
		if(origin == SEEK_CUR)
			o = p_sb->GetRdOffs() + offs;
		else if(origin == SEEK_END)
			o = p_sb->GetWrOffs() + offs;
		else
			o = offs;
		if(checkirange(o, 0LL, static_cast<int64>(p_sb->GetWrOffs()))) {
			p_sb->SetRdOffs(static_cast<size_t>(o));
			ok = true;
		}
		else
			ok = false;
	}
	else {
		if(T == tArchive) {
			if(!!H) {
				ok = LOGIC(SArchive::SeekArchiveEntry(H, offs, origin));
			}
			else
				ok = false;
		}
		else {
			FILE * p_f = GetFilePtr();
			if(p_f) {
	#if (_MSC_VER >= 1900)
				ok = (_fseeki64(p_f, offs, origin) == 0);
	#else
				assert(offs < MAXLONG);
				ok = (fseek(p_f, static_cast<long>(offs), origin) == 0);
	#endif
			}
			else if(IH >= 0) {
				ok = (_lseeki64(IH, offs, origin) >= 0);
			}
			else
				ok = false;
		}
	}
	BufR.Z();
	return ok;
}

long SFile::Tell()
{
	assert(InvariantC(0));
	long   t = 0;
	SBuffer * p_sb = GetSBufPtr();
	if(p_sb) {
		t = static_cast<long>(p_sb->GetRdOffs());
	}
	else {
		FILE * p_f = GetFilePtr();
		if(p_f)
			t = ftell(p_f);
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
	SBuffer * p_sb = GetSBufPtr();
	if(p_sb) {
		t = static_cast<int64>(p_sb->GetRdOffs());
	}
	else {
		FILE * p_f = GetFilePtr();
		if(p_f) {
#if (_MSC_VER >= 1900)
			t = _ftelli64(p_f);
#else
			t = ftell(p_f);
#endif
		}
		else if(IH >= 0) {
			t = _telli64(IH);
			if(t >= 0) {
				const int64 bo = static_cast<int64>(BufR.GetWrOffs());
				assert(bo <= t);
				if(bo <= t)
					t -= bo;
			}
		}
		else {
			SLS.SetError(SLERR_FILENOTOPENED); // @v11.3.7
			t = -1LL; // @v11.3.7
			// @v11.3.7 t = (SLibError = SLERR_FILENOTOPENED, 0);
		}
	}
	return t;
}

int SFile::Flush()
{
	int    ok = -1;
	FILE * p_f = GetFilePtr();
	if(p_f)
		ok = BIN(fflush(p_f) == 0);
	return ok;
}

int SFile::Write(const void * pBuf, size_t size)
{
	assert(InvariantC(0));
	int    ok = 1;
	if(T == tNullOutput)
		ok = 1;
	else if(T == tArchive)
		ok = 0;
	else {
		SBuffer * p_sb = GetSBufPtr();
		if(p_sb)
			ok = p_sb->Write(pBuf, size);
		else {
			FILE * p_f = GetFilePtr();
			if(p_f)
				ok = (fwrite(pBuf, size, 1, p_f) == 1) ? 1 : SLS.SetError(SLERR_WRITEFAULT, Name);
			else if(IH >= 0)
				ok = (write(IH, pBuf, size) == size) ? 1 : SLS.SetError(SLERR_WRITEFAULT, Name);
			else
				ok = (SLibError = SLERR_FILENOTOPENED, 0);
		}
	}
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
	assert(size < MAXINT32); // @v11.3.7
	int    ok = 1;
	int    act_size = 0;
	SBuffer * p_sb = GetSBufPtr();
	THROW_S(T != tNullOutput, SLERR_SFILRDNULLOUTP);
	if(p_sb) {
		act_size = p_sb->Read(pBuf, size);
	}
	else {
		if(T == tArchive) {
			if(!!H) {
				size_t act_size_u;
				ok = SArchive::ReadArchiveEntry(H, pBuf, size, &act_size_u);
				if(ok) {
					if(act_size_u == 0)
						ok = -1;
					act_size = static_cast<int>(act_size_u);
				}
			}
		}
		else {
			FILE * p_f = GetFilePtr();
			if(p_f) {
				const int64 offs = Tell64();
				if(fread(pBuf, size, 1, p_f) == 1)
					act_size = static_cast<int>(size);
				else {
					//
					// Для того чтобы функция считала последний блок из файла, если он не равен size
					//
					Seek64(offs, SEEK_SET);
					act_size = static_cast<int>(fread(pBuf, 1, size, p_f));
					if(!act_size) {
						if(feof(p_f))
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
							if(local_act_size < static_cast<int>(temp_buf.GetSize())) {
								is_eof = 1;
								if(local_act_size == 0)
									ok = -1;
							}
						}
						else {
							const int local_act_size = read(IH, pBuf, size_to_do);
							THROW_S_S(local_act_size >= 0, SLERR_READFAULT, Name);
							act_size += local_act_size;
							if(local_act_size < static_cast<int>(size_to_do)) {
								is_eof = 1;
								ok = -1;
							}
							size_to_do -= local_act_size;
						}
					}
				}
			}
			else {
				THROW_S_S(p_f || (IH >= 0), SLERR_FILENOTOPENED, Name);
			}
		}
	}
	CATCH
		act_size = 0;
		ok = 0;
	ENDCATCH
	ASSIGN_PTR(pActualSize, static_cast<size_t>(act_size));
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
		SBuffer * p_sb = GetSBufPtr();
		if(p_sb)
			ok = p_sb->Write(pBuf, size_to_write);
		else {
			FILE * p_f = GetFilePtr();
			if(p_f)
				ok = (fputs(pBuf, p_f) >= 0) ? 1 : SLS.SetError(SLERR_WRITEFAULT, Name);
			else if(IH >= 0) {
				ok = (write(IH, pBuf, size_to_write) == size_to_write) ? 1 : SLS.SetError(SLERR_WRITEFAULT, Name);
			}
			else
				ok = (SLibError = SLERR_FILENOTOPENED, 0);
		}
	}
	return ok;
}

SFile::ReadLineCsvContext::ReadLineCsvContext(char fieldDivider) : FieldDivider(fieldDivider)
{
}

uint SFile::ReadLineCsvContext::ImplementScan(const char * pLine, StringSet & rSs)
{
	rSs.Z();
	uint  result = 0;
	if(FieldDivider) {
		if(pLine) {
			Scan.Set(pLine, 0);
		}
		else {
			Scan.Set(LineBuf, 0);
		}
		{
			bool   last_divider = false;
			while(!Scan.IsEnd()) {
				FldBuf.Z();
				if(Scan[0] == FieldDivider) {
					Scan.Incr();
					last_divider = true;
				}
				else {
					last_divider = false;
					if(Scan[0] == '\"') {
						THROW(Scan.GetQuotedString(SFileFormat::Csv, FldBuf));
						if(Scan.Skip(1)[0] == FieldDivider) {
							Scan.Incr();
							last_divider = true;
						}
					}
					else {
						Scan.GetUntil(FieldDivider, FldBuf);
						if(Scan[0] == FieldDivider) {
							Scan.Incr();
							last_divider = true;
						}
					}
				}
				// Если StringSet не имеет явного разделителя, то придется вместо пустых полей вставлять пробелы, иначе вызывающая сторона не
				// сможет правильно обработать результат.
				if(rSs.isZeroDelim() && FldBuf.IsEmpty()) 
					FldBuf.Space();
				rSs.add(FldBuf);
				++result;
			}
			// Если в конце строки стоял разделитель, то мы имеем пустое поле в конце записи. Учтем это.
			if(last_divider) {
				FldBuf.Z();
				if(rSs.isZeroDelim()) 
					FldBuf.Space();
				rSs.add(FldBuf);
				++result;
			}
		}
	}
	CATCH
		result = 0;
	ENDCATCH
	return result;
}

int SFile::ReadLineCsv(ReadLineCsvContext & rCtx, StringSet & rSs)
{
	assert(InvariantC(0));
	rSs.Z();
	int    ok = 0;
	if(rCtx.FieldDivider) {
		rCtx.LineBuf.Z();
		if(ReadLine(rCtx.LineBuf, rlfChomp|rlfStrip)) {
			if(rCtx.ImplementScan(0, rSs))
				ok = 1;
			else
				ok = -1;
		}
	}
	return ok;
}

int SFile::ReadLine(SString & rBuf) { return ReadLine(rBuf, 0); }

int SFile::ReadLine(SString & rBuf, uint flags)
{
	assert(InvariantC(0));
	int    ok = 1;
	rBuf.Z();
	switch(T) {
		case tNullOutput:
			CALLEXCEPT_S(SLERR_SFILRDNULLOUTP);
			break;
		case tStdFile:
			{
				FILE * p_f = GetFilePtr();
				THROW_S(p_f, SLERR_FILENOTOPENED);
				{
					THROW(LB.Alloc(1024));
					//
					// На случай, если строка в файле длиннее, чем buf_size
					// считываем ее в цикле до тех пор, пока в конце строки не появится //
					// перевод каретки.
					//
					char * p = 0;
					while((p = fgets(LB, LB.GetSize(), p_f)) != 0) {
						rBuf.Cat(LB);
						size_t len = sstrlen(LB);
						if(LB[len-1] == '\n' || (LB[len-2] == 0x0D && LB[len-1] == 0x0A))
							break;
					}
					THROW_S_S(rBuf.Len() || p, SLERR_READFAULT, Name);
				}
			}
			break;
		case tArchive:
			if(!!H) {
				int   rr = 0;
				int   _done = 0;
				do {
            		int8  rd_buf[16];
					size_t act_size = 0;
					THROW(rr = Read(rd_buf, 1, &act_size));
					if(act_size) {
						if(rd_buf[act_size-1] == '\n' || (rd_buf[act_size-1] == '\x0A' && rBuf.Last() == '\x0D'))
							_done = 1;
						rBuf.CatN(reinterpret_cast<const char *>(rd_buf), act_size);
					}
				} while(rr > 0 && !_done);
				THROW_S_S(rBuf.Len(), SLERR_READFAULT, Name);				
			}
			break;
		case tSBuffer:
		case tFile:
			THROW_S(T != tFile || IH >= 0, SLERR_FILENOTOPENED);
			if(IH >= 0 && Mode & SFile::mBuffRd) {
				int   rr = 0;
				int   _done = 0;
				do {
            		int8  rd_buf[16];
					size_t act_size = 0;
					THROW(rr = Read(rd_buf, 1, &act_size));
					if(act_size) {
						if(rd_buf[act_size-1] == '\n' || (rd_buf[act_size-1] == '\x0A' && rBuf.Last() == '\x0D'))
							_done = 1;
						rBuf.CatN(reinterpret_cast<const char *>(rd_buf), act_size);
					}
				} while(rr > 0 && !_done);
				THROW_S_S(rBuf.Len(), SLERR_READFAULT, Name);
			}
			else {
				int64  last_pos = Tell64();
				const char * p = 0;
				THROW(LB.Alloc(1024+2));
				size_t act_size = 0;
				LB[LB.GetSize()-2] = 0;
				while(Read(LB, LB.GetSize()-2, &act_size) && act_size) {
					p = static_cast<const char *>(smemchr(LB.vptr(), '\n', act_size)); // @v11.7.0 memchr-->smemchr
					if(p) {
						char * p_to_update = const_cast<char *>(p);
						p_to_update[1] = 0;
					}
					else {
						p = static_cast<const char *>(smemchr(LB.vptr(), 0x0D, act_size)); // @v11.7.0 memchr-->smemchr
						// @v10.4.1 @fix 
						if(p) {
							char * p_to_update = const_cast<char *>(p);
							if(p_to_update[1] == 0x0A)
								p_to_update[2] = 0;
							else
								p_to_update[1] = 0;
						}
					}
					if(p) {
						const size_t _len = sstrlen(LB.cptr());
						rBuf.CatN(LB, _len);
						Seek64(last_pos + _len);
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
			break;
		default:
			CALLEXCEPT_S(SLERR_INVALIDSFILTYP);
			break;
	}
	// @v11.4.3 {
	assert(ok == 1);
	if(flags & rlfChomp)
		rBuf.Chomp();
	if(flags & rlfStrip)
		rBuf.Strip();
	// } @v11.4.3 
	CATCHZOK
	return ok;
}

int SFile::ReadAll(STempBuffer & rBuf, size_t maxSize, size_t * pActualSize)
{
	int    ok = 1;
	size_t actual_size = 0;
	THROW(IsValid());
	{
		const int64 current_pos = Tell64();
		THROW(Seek64(0, SEEK_END));
		{
			const int64 end_pos = Tell64();
			assert(end_pos >= current_pos);
			THROW(end_pos >= current_pos);
			THROW(Seek64(current_pos, SEEK_SET));
			if(end_pos == current_pos) {
				ok = -1;
			}
			else {
				const int64 insurance = 64; // Страховочный "хвостик" в конце буфера
				int64 rest_size = (end_pos - current_pos);
				if(maxSize)
					SETMIN(rest_size, static_cast<int64>(maxSize));
				THROW((rest_size+insurance) < UINT_MAX);
				if((rest_size+insurance) > static_cast<int64>(rBuf.GetSize())) {
					THROW(rBuf.Alloc(static_cast<size_t>(rest_size+insurance))); // Преобразование (to size_t) корректно из-за оператора выше (THROW(rest_size < MAXSIZE_T)).
				}
				THROW(Read(rBuf, static_cast<size_t>(rest_size), &actual_size)); // Преобразование (to size_t) корректно из-за оператора выше (THROW(rest_size < MAXSIZE_T)).
				// Мы все предварительно расчитали, следовательно rest_size должен быть строго равен actual_size, НО
				// с файлом может что-то происходить параллельно, могут быть проблемы с трактовкой символов перевода строки и т.д.
				// По-этому, от соблазна строгой проверки уклонимся.
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pActualSize, actual_size);
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
		if(!r) {
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
			if(!r) {
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

bool SFile::CalcSize(int64 * pSize)
{
	assert(InvariantC(0));
	bool   ok = true;
	int64  sz = 0;
	SBuffer * p_sb = GetSBufPtr();
	if(p_sb)
		sz = p_sb->GetWrOffs();
	else {
		THROW(IsValid());
		{
			const int64  save_pos = Tell64();
			THROW(Seek64(0, SEEK_END));
			sz = Tell64();
			THROW(Seek64(save_pos, SEEK_SET));
		}
	}
	CATCHZOK
	ASSIGN_PTR(pSize, sz);
	return ok;
}

int SFile::GetDateTime(LDATETIME * pCreate, LDATETIME * pLastAccess, LDATETIME * pModif)
{
	if(T == tSBuffer)
		return -1;
	else if(IsValid()) {
		FILE * p_f = GetFilePtr();
		if(p_f) {
			return SFile::GetTime(SIntHandle(_get_osfhandle(fileno(p_f))), pCreate, pLastAccess, pModif);
		}
		else if(IH >= 0) {
			return SFile::GetTime(SIntHandle(_get_osfhandle(IH)), pCreate, pLastAccess, pModif);
		}
		else
			return -1;
	}
	else
		return 0;
}

int SFile::SetDateTime(LDATETIME * pCreate, LDATETIME * pLastAccess, LDATETIME * pModif)
{
	int    ok = 0;
	if(T == tSBuffer)
		ok = -1;
	else if(IsValid()) {
		FILE * p_f = GetFilePtr();
		if(p_f)
			ok = SFile::SetTime(SIntHandle(_get_osfhandle(fileno(p_f))), pCreate, pLastAccess, pModif);
		else if(IH >= 0)
			ok = SFile::SetTime(SIntHandle(_get_osfhandle(IH)), pCreate, pLastAccess, pModif);
		else
			ok = -1;
	}
	return ok;
}

int SFile::SetDateTime(int64 tmNs100Creation, int64 tmNs100LastAccess, int64 tmNs100LastModif)
{
	int    ok = 0;
	if(T == tSBuffer)
		ok = -1;
	else if(IsValid()) {
		FILE * p_f = GetFilePtr();
		if(p_f)
			ok = SFile::SetTime(SIntHandle(_get_osfhandle(fileno(p_f))), tmNs100Creation, tmNs100LastAccess, tmNs100LastModif);
		else if(IH >= 0)
			ok = SFile::SetTime(SIntHandle(_get_osfhandle(IH)), tmNs100Creation, tmNs100LastAccess, tmNs100LastModif);
		else
			ok = -1;
	}
	return ok;
}

bool SFile::CalcHash(int64 offs, int hashFuncIdent/* SHASHF_XXX */, SBinaryChunk & rHash)
{
	bool   ok = true;
	const  int64  preserve_pos = Tell64();
	SlHash::State st;
	THROW_S(oneof5(hashFuncIdent, SHASHF_SHA1, SHASHF_SHA256, SHASHF_SHA512, SHASHF_MD5, SHASHF_CRC32), SLERR_INVORUNSUPPHASHFUNC);
	THROW(Seek64(0, SEEK_END));
	{
		const int64 file_size = Tell64();
		const int64 target_size = file_size - offs;
		THROW(file_size >= 0LL);
		THROW(target_size >= 0);
		{
			const size_t nominal_rd_buf_size = SMEGABYTE(4);
			STempBuffer rd_buf(nominal_rd_buf_size+128/*isurance*/);
			size_t actual_rd_size = 0;
			THROW(rd_buf.IsValid());
			THROW(Seek64(offs, SEEK_SET));
			do {
				THROW(Read(rd_buf, nominal_rd_buf_size, &actual_rd_size));
				switch(hashFuncIdent) {
					case SHASHF_SHA1: SlHash::Sha1(&st, rd_buf, actual_rd_size); break;
					case SHASHF_SHA256: SlHash::Sha256(&st, rd_buf, actual_rd_size); break;
					case SHASHF_SHA512: SlHash::Sha512(&st, rd_buf, actual_rd_size); break;
					case SHASHF_MD5: SlHash::Md5(&st, rd_buf, actual_rd_size); break;
					case SHASHF_CRC32: SlHash::CRC32(&st, rd_buf, actual_rd_size); break;
					default: 
						assert(0); 
						CALLEXCEPT_S(SLERR_INVORUNSUPPHASHFUNC);
						break;
				}
			} while(actual_rd_size == nominal_rd_buf_size);
			switch(hashFuncIdent) {
				case SHASHF_SHA1: 
					{
						binary160 result = SlHash::Sha1(&st, 0, 0);
						rHash.Put(&result, sizeof(result));
					}
					break;
				case SHASHF_SHA256: 
					{
						binary256 result = SlHash::Sha256(&st, 0, 0); 
						rHash.Put(&result, sizeof(result));
					}
					break;
				case SHASHF_SHA512: 
					{
						binary512 result = SlHash::Sha512(&st, 0, 0); 
						rHash.Put(&result, sizeof(result));
					}
					break;
				case SHASHF_MD5: 
					{
						binary128 result = SlHash::Md5(&st, 0, 0); 
						rHash.Put(&result, sizeof(result));
					}
					break;
				case SHASHF_CRC32: 
					{
						uint32 result = SlHash::CRC32(&st, 0, 0);
						rHash.Put(&result, sizeof(result));
					}
					break;
				default: 
					assert(0); 
					CALLEXCEPT_S(SLERR_INVORUNSUPPHASHFUNC);
			}
		}
	}
	CATCHZOK
	Seek64(preserve_pos, SEEK_SET);
	return ok;
}

int SFile::CalcCRC(long offs, uint32 * pCrc)
{
	int    ok = 1;
	SCRC32 c;
	uint32 crc = 0;
	const  int64  preserve_pos = Tell64();
	Seek64(0, SEEK_END);
	int64  sz = Tell64();
	//if(sz > 0) {
	THROW(sz >= 0);
	THROW(sz >= offs);
	sz -= offs;
	{
		const  size_t blk_size = SKILOBYTE(32);
		const  int64  num_blk = sz / blk_size;
		size_t rest = static_cast<size_t>(sz % blk_size);
		STempBuffer temp_buf(blk_size);
		THROW(Seek64(offs, SEEK_SET));
		for(int64 i = 0; i < num_blk; i++) {
			size_t actual_size = 0;
			THROW(Read(temp_buf, temp_buf.GetSize(), &actual_size));
			crc = c.Calc(crc, temp_buf.cptr(), temp_buf.GetSize());
		}
		if(rest > 0) {
			THROW(Read(temp_buf, rest));
			crc = c.Calc(crc, temp_buf.cptr(), rest);
		}
	}
	CATCHZOK
	Seek64(preserve_pos, SEEK_SET);
	ASSIGN_PTR(pCrc, crc);
	return ok;
}
//
// Идентификация форматов файлов
//
class FileFormatRegBase : private SVector, public SStrGroup {
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
	//
	// Descr: Функция идентифицирует формат данных, находящихся в буфере pBuf.
	//  Функция пытается идентифицировать формат по сигнатуре, полагая, что буфер является образом файла,
	//  который может быть идентифицирован функцией Identify(const char * pFileName, int * pFmtId, SString * pExt) (see above).
	// Returns:
	//   2 - формат идентифицироан по сигнатуре
	//   4 - формат не удалось идентифицировать по сигнатуре, однако
	//     по начальному блоку данных он похож на результирующий формат.
	//     На текущий момент такой вариант возможен для форматов: SFileFormat::TxtAscii, SFileFormat::TxtUtf8, SFileFormat::Txt
	//  -1 - не удалось идентифицировать формат
	//
	int    IdentifyBuffer(const void * pBuf, size_t bufLen, int * pFmtId, SString * pExt) const;
	int    IdentifyMime(const char * pMime, int * pFmtId) const;
	int    GetMimeType(int id) const;
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

int FileFormatRegBase::GetMimeType(int id) const
{
	int    mime_type = 0;
	for(uint pos = 0; !mime_type && SVector::lsearch(&id, &pos, CMPF_LONG); pos++) {
		const Entry * p_entry = static_cast<const Entry *>(at(pos));
		assert(p_entry->FmtId == id);
		if(p_entry->MimeType)
			mime_type = p_entry->MimeType;
	}
	return mime_type;
}

int FileFormatRegBase::GetMime(int id, SString & rMime) const
{
	rMime.Z();
	int    ok = 0;
	for(uint pos = 0; !ok && SVector::lsearch(&id, &pos, CMPF_LONG); pos++) {
		const Entry * p_entry = static_cast<const Entry *>(at(pos));
		assert(p_entry->FmtId == id);
		if(p_entry->MimeType && p_entry->MimeSubtypeIdx) {
			SFileFormat::GetMimeTypeName(p_entry->MimeType, rMime);
			SString & r_temp_buf = SLS.AcquireRvlStr();
			GetS(p_entry->MimeSubtypeIdx, r_temp_buf);
			rMime.Slash().Cat(r_temp_buf);
			ok = 1;
		}
		// @v10.2.12 @fix break;
	}
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
				if(temp_buf.HasChr(';')) {
					StringSet ss(';', temp_buf);
					if(ss.get(0U, temp_buf)) {
						rExt = temp_buf;
						ok = 1;
					}
				}
				else {
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

int FileFormatRegBase::IdentifyBuffer(const void * pBuf, size_t bufLen, int * pFmtId, SString * pExt) const
{
	int    ok = -1;
	int    fmt_id = 0;
	//LongArray candid_by_ext;
	LongArray candid_by_sign;
	if(bufLen) {
		//const SFsPath ps(pFileName);
		int    entry_mime_type;
		//SString ext = ps.Ext;
		SString entry_ext;
		SString entry_sign;
		SString entry_mime_subtype;
		SString temp_buf;
		SString left_buf;
		SString right_buf;
		StringSet ss_subsigns;
		LongArray used_offs_list;
		StrAssocArray binary_chunk_list;
		//ASSIGN_PTR(pExt, ext);
		//STempBuffer sign_buf(512);
		//SFile file(pFileName, SFile::mRead|SFile::mBinary);
		//const int64  _fsize = static_cast<int64>(bufLen);
		//if(file.IsValid())
		//	file.CalcSize(&_fsize);
		//ext.Strip().ShiftLeftChr('.').Strip().ToLower();
		for(uint i = 0; i < getCount(); i++) {
			Entry entry;
			if(Get(i, entry, entry_ext, entry_sign, entry_mime_type, entry_mime_subtype)) {
				/*if(entry_ext.NotEmpty() && entry_ext == ext) {
					candid_by_ext.addUnique(entry.FmtId);
				}*/
				if(entry_sign.NotEmpty()) {
                    if(entry_sign.C(0) == 'T') {
						entry_sign.ShiftLeft(1);
						const size_t len = 512;
						const size_t actual_size = bufLen;
						//THROW(sign_buf.Alloc(len));
						//file.Seek(0);
						//assert(len <= sign_buf.GetSize());
						if(len <= bufLen /*&& file.Read(sign_buf, len, &actual_size)*/) {
							int    r = -1;
							size_t j = 0;
							//"EFBBBF"
							if(PTR8C(pBuf)[0] == 0xEF && PTR8C(pBuf)[1] == 0xBB && PTR8C(pBuf)[2] == 0xBF) // BOM UTF8 
								j += 3;
							while(r < 0 && j < actual_size) {
								const char c = PTRCHRC(pBuf)[j];
								if(oneof4(c, ' ', '\t', '\x0D', '\x0A')) {
                                    j++;
								}
								else if(entry_sign.CmpL(PTRCHRC(pBuf)+j, 1) == 0) // @v10.0.02 @fix sign_buf-->sign_buf+j
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
						ss_subsigns.Z();
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
						if(checkirange(total_len, static_cast<size_t>(1), static_cast<size_t>(512))) {
							const size_t len = total_len;
							const size_t actual_size = MIN(bufLen, len);
							//THROW(sign_buf.Alloc(len));
							//file.Seek(0);
							//assert(len <= sign_buf.GetSize());
							if(actual_size == len) {
								int r = 1;
								for(uint ci = 0; r && ci < binary_chunk_list.getCount(); ci++) {
									StrAssocArray::Item bcl_item = binary_chunk_list.at_WithoutParent(ci);
									const size_t item_len = sstrlen(bcl_item.Txt) / 2;
									const size_t offs = bcl_item.Id;
									for(size_t j = 0; r && j < item_len; j++) {
										const uint8 file_byte = PTR8C(pBuf)[offs+j];
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
				/*else if(entry.SignFunc) {
					if(entry.SignFunc(file, 0) > 0) {
						candid_by_sign.addUnique(entry.FmtId);
					}
				}*/
			}
		}
		{
			/*for(uint cidx = 0; !fmt_id && cidx < candid_by_ext.getCount(); cidx++) {
				const long ext_id = candid_by_ext.get(cidx);
				for(uint j = 0; !fmt_id && j < candid_by_sign.getCount(); j++)
					if(ext_id == candid_by_sign.get(j))
						fmt_id = ext_id;
			}*/
			if(fmt_id)
				ok = 3;
			else if(candid_by_sign.getCount()) {
				fmt_id = candid_by_sign.get(0);
				ok = 2;
			}
			/*else if(candid_by_ext.getCount()) {
				fmt_id = candid_by_ext.get(0);
				ok = 1;
			}*/
			else {
				STextEncodingStat tes;
				const size_t actual_size = bufLen;
				//STempBuffer tbuf(1024);
				//if(tbuf.IsValid() && file.Read(tbuf, tbuf.GetSize(), &actual_size)) {
				{
					tes.Add(pBuf, actual_size);
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
	//CATCHZOK
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
		const SFsPath ps(pFileName);
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
							ss_subsigns.Z();
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
							if(checkirange(total_len, static_cast<size_t>(1), static_cast<size_t>(512))) {
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

void SFileFormat::Clear() { Id = Unkn; }
SFileFormat::operator int () const { return Id; }
int SFileFormat::operator !() const { return (Id == 0); }

int SFileFormat::GetMimeType() const
{
	int    mime_type = 0;
	if(Id && GloBaseIdx) {
		const FileFormatRegBase * p_reg = static_cast<const FileFormatRegBase *>(SLS.GetGlobalObject(GloBaseIdx));
		if(p_reg)
			mime_type = p_reg->GetMimeType(Id);
	}
	return mime_type;
}

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

int SFileFormat::IdentifyBuffer(const void * pBuf, size_t bufLen)
{
	int    ok = 0;
	Id = Unkn;
	if(pBuf && bufLen) {
		const FileFormatRegBase * p_reg = static_cast<const FileFormatRegBase *>(SLS.GetGlobalObject(GloBaseIdx));
		if(p_reg)
			ok = p_reg->IdentifyBuffer(pBuf, bufLen, &Id, 0);
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
	Register(Otf,    mtFont,        "opentype", "otf",      "4F54544F00"); // @v11.3.7 OpenType font file
	Register(Ttf,    mtApplication, "x-font-ttf",  "ttf",   "0001000000"); // @v11.3.7 TrueType font file // ! Ttf стоит перед Ico из-за похожих сигнатур
	Register(Ttc,    mtApplication, "x-font-ttf",  "ttc",   "7474636600"); // @v11.3.7 TrueType font Collection
	Register(Dfont,  mtFont,        "ttf",         "dfont", "0000010000"); // @v11.3.7 Mac OS X Data Fork Font
	Register(Ico,    mtImage, "x-icon", "ico", "0001");             // ICO
	Register(Cur,    mtApplication, "octet-stream", "cur", "0002"); // CUR
	Register(Xml,    mtApplication, "xml", "xml", "T<?xml");        // XML
	Register(Xsd,    mtApplication, "xml", "xsd", "T<?xml");        // XSD @v10.8.0
	Register(Svg,    mtImage, "svg+xml",   "svg", "T<?xml"); // SVG // @todo Необходимо проверить XML-контент на наличие тега <svg>
	Register(Html,   mtText,  "html",  "html;htm", "T<!DOCTYPE HTML"); // HTML
	Register(Ini,    mtText,  "plain", "ini", static_cast<const char *>(0));  // INI
	Register(Csv,    mtText,  "csv",   "csv", 0); // @v10.8.0
	Register(Tsv,    mtText,  "tsv",   "tsv", 0); // @v10.9.9 tab-separated-values
	Register(Latex,  mtApplication, "x-latex", "tex", static_cast<const char *>(0));  // LATEX
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

class CsvSniffer {
public:
	CsvSniffer();
	~CsvSniffer();
	int    Run(const char * pFileName, SFileFormat::CsvSinffingResult & rR);
};

CsvSniffer::CsvSniffer()
{
}
	
CsvSniffer::~CsvSniffer()
{
}

int CsvSniffer::Run(const char * pFileName, SFileFormat::CsvSinffingResult & rR) // @construction
{
	rR.FieldDivisor = 0;
	int    ok = -1;
	uint   max_lines = 100;
	THROW(!isempty(pFileName));
	{
		SString line_buf;
		SFile f_in(pFileName, SFile::mRead);
		uint   line_no = 0;
		uint   empty_line_count = 0;
		uint32 common_seq = _FFFF32; // 
		uint32 first_line_seq = _FFFF32;
		STokenRecognizer tr;
		SNaturalTokenArray nta;
		TSCollection <SNaturalTokenStat> nts_list;
		SStrCollection line_list;
		while(line_no < max_lines && f_in.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
			if(line_buf.IsEmpty())
				empty_line_count++;
			else {
				line_no++;
				line_list.insert(newStr(line_buf));
				SNaturalTokenStat * p_nts = nts_list.CreateNewItem();
				THROW(p_nts);
				THROW(tr.Run(line_buf.ucptr(), line_buf.Len(), nta, p_nts));
				common_seq &= p_nts->Seq;
				if(line_no == 1)
					first_line_seq = p_nts->Seq;
			}
		}
		rR.SnTokSeq_Common = common_seq;
		rR.SnTokSeq_First = first_line_seq;
		{
			struct PotentialDivisor {
				PotentialDivisor(const char c) : C(c)
				{
				}
				const char C;
				StatBase S;
			};
			TSCollection <PotentialDivisor> potential_div_list;
			potential_div_list.insert(new PotentialDivisor(','));
			potential_div_list.insert(new PotentialDivisor(';'));
			potential_div_list.insert(new PotentialDivisor('\t'));
			potential_div_list.insert(new PotentialDivisor('|'));
			{
				// Собираем статистику появления потенциального символа-разделителя.
				for(uint pdidx = 0; pdidx < potential_div_list.getCount(); pdidx++) {
					PotentialDivisor * p_pd = potential_div_list.at(pdidx);
					assert(p_pd);
					if(p_pd) {
						for(uint ntsidx = 0; ntsidx < nts_list.getCount(); ntsidx++) {
							const SNaturalTokenStat * p_nts = nts_list.at(ntsidx);
							assert(p_nts);
							if(p_nts) {
								uint cp = 0;
								if(p_nts->ChrList.BSearch(p_pd->C, &cp)) {
									p_pd->S.Step(p_nts->ChrList.at(cp).Val);
								}
							}
						}
						p_pd->S.Finish();
					}
				}
			}
			{
				// Анализируем статистику появления потенциального символа-разделителя.
				RAssoc max_exp_to_dev_rel;
				max_exp_to_dev_rel.Key = -1;
				max_exp_to_dev_rel.Val = 0.0;
				for(uint pdidx = 0; pdidx < potential_div_list.getCount(); pdidx++) {
					const PotentialDivisor * p_pd = potential_div_list.at(pdidx);
					assert(p_pd);
					if(p_pd && p_pd->S.GetCount()) {
						//
						// Наиболее вероятным разделителем считаем тот, у которого отношение 
						// среднего числа символом на строку к стандартному отклонению распределения //
						// числа разделителей на строку максимальное.
						// В идеальном случае среднее равно числу полей минус один, а отклонение - нулю 
						// но существуют девиации (поля в кавычках, содержащие разделитель; ошибки формирования файла и т.д.)
						//
						double exp = p_pd->S.GetExp();
						double sd = p_pd->S.GetStdDev();
						assert(exp > 0.0);
						if(sd == 0.0) {
							max_exp_to_dev_rel.Key = pdidx;
							max_exp_to_dev_rel.Val = 1000000.0;
						}
						else {
							if(max_exp_to_dev_rel.Val < (exp / sd)) {
								max_exp_to_dev_rel.Key = pdidx;
								max_exp_to_dev_rel.Val = (exp / sd);
							}
						}
					}
				}
				if(max_exp_to_dev_rel.Key >= 0) {
					rR.FieldDivisor = potential_div_list.at(max_exp_to_dev_rel.Key)->C;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SEncodingFormat::SEncodingFormat(int f) : Id(oneof2(f, Unkn, Base64) ? f : 0)
{
}

SEncodingFormat::operator int () const { return Id; }
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
SCachedFileEntity::SCachedFileEntity() : State(0), ModTime(ZERODATETIME)
{
}

/*virtual*/SCachedFileEntity::~SCachedFileEntity()
{
}

int SCachedFileEntity::Init(const char * pFilePath)
{
	int    ok = 0;
	ENTER_CRITICAL_SECTION
	if(State & stInitialized) {
		ok = -1;
	}
	else if(!isempty(pFilePath)) {
		SFsPath::NormalizePath(pFilePath, SFsPath::npfSlash|SFsPath::npfCompensateDotDot, FilePath);
		if(fileExists(FilePath)) {
			State |= stInitialized;
			State &= ~stError;
			ok = 1;
		}
		else {
			FilePath.Z();
			State |= stError;
		}
	}
	else
		State |= stError;
	LEAVE_CRITICAL_SECTION
	return ok;
}

bool SCachedFileEntity::InitFileModTime()
{
	return (FilePath.NotEmpty() && SFile::GetTime(FilePath, 0, 0, &ModTime));
}

bool SCachedFileEntity::IsModified()
{
	LDATETIME current_mod_time = ZERODATETIME;
	if(FilePath.NotEmpty() && SFile::GetTime(FilePath, 0, 0, &current_mod_time)) {
		return (!ModTime || cmp(current_mod_time, ModTime) > 0);
	}
	else
		return false;
}

const void * SCachedFileEntity::GetHashKey(const void * pCtx, uint * pSize) const // hash-table support
{
	ASSIGN_PTR(pSize, FilePath.Len());
	return FilePath.ucptr();
}

const char * SCachedFileEntity::GetFilePath() const { return FilePath; }

/*virtual*/bool SCachedFileEntity::InitEntity(void * extraPtr)
{
	return false;
}

/*virtual*/void SCachedFileEntity::DestroyEntity()
{
}

int SCachedFileEntity::Reload(bool force, void * extraPtr)
{
	int    ok = -1;
	bool   do_load = false;
	LDATETIME current_mod_time = ZERODATETIME;
	Lck.Lock();
	if(force)
		do_load = true;
	else {
		if(FilePath.IsEmpty())
			ok = 0;
		else if(!SFile::GetTime(FilePath, 0, 0, &current_mod_time)) {
			ok = 0;
		}
		else if(!ModTime || cmp(current_mod_time, ModTime) > 0)
			do_load = true;
	}
	if(do_load) {
		DestroyEntity();
		if(InitEntity(extraPtr)) {
			ok = 1;
			if(!current_mod_time) {
				if(SFile::GetTime(FilePath, 0, 0, &current_mod_time)) {
					ModTime = current_mod_time;
				}
				else {
					// Фантастический сценарий: мы успешно проделали все шаги по проверки и загрузке файла, но
					// споткнулись на получении времени модификации. Скорее всего этого не будет, но если да,
					// то не будем инициировать ошибку, но обнулим время модификации.
					ModTime = ZERODATETIME;
				}
			}
		}
		else {
			DestroyEntity();
			State |= stError;
		}
	}
	Lck.Unlock();
	return ok;
}
//
//
//
class SFileEntityCache : public TSHashCollection <SCachedFileEntity> { // @v11.7.5
public:
	SFileEntityCache() : TSHashCollection <SCachedFileEntity> (2048, 0)
	{
	}
	~SFileEntityCache()
	{
	}
};
//
//
//
static bool GetSelfSid(SID * pSid, DWORD * pSidSize)
{
	wchar_t user[128] = {};
	wchar_t sys[128] = {};
	wchar_t domain[128] = {};
	DWORD user_size = SIZEOFARRAY(user);
	DWORD domain_size = SIZEOFARRAY(domain);
	if(!::GetUserNameW(user, &user_size)) {
		return false;
	}
	else {
		SID_NAME_USE snu = SidTypeUser;
		return ::LookupAccountNameW(sys, user, pSid, pSidSize, domain, &domain_size, &snu);
	}
}

static ACL * MyselfAcl()
{
	ACL * p_acl = 0;
	BYTE sid_buf[512];
	SID * p_sid = (SID *)sid_buf;
	DWORD size = sizeof(sid_buf);
	if(GetSelfSid(p_sid, &size)) {
		DWORD acl_size = 512;
		p_acl = (ACL *)SAlloc::M(acl_size);
		::InitializeAcl(p_acl, acl_size, ACL_REVISION);
		::AddAccessAllowedAce(p_acl, ACL_REVISION, GENERIC_ALL, p_sid);
	}
	return p_acl;
}

bool ResetAcl(const wchar_t * pPath, bool myselfAcl)
{
	static ACL default_acl;
	static BOOL once_result = ::InitializeAcl(&default_acl, sizeof(default_acl), ACL_REVISION);
	ACL * p_acl = once_result ? &default_acl : NULL;
	if(myselfAcl) {
		static ACL * local_acl = MyselfAcl();
		if(local_acl)  
			p_acl = local_acl;
	}
	return p_acl ? (::SetNamedSecurityInfoW(const_cast<wchar_t *>(pPath), SE_FILE_OBJECT, 
		DACL_SECURITY_INFORMATION|UNPROTECTED_DACL_SECURITY_INFORMATION, 0, 0, p_acl, 0) == ERROR_SUCCESS) : false;
}

SIntHandle SFile::ForceCreateFile(const wchar_t * pPath, uint mode, uint share, /*SECURITY_ATTRIBUTES*/void * pSa, uint crMode, uint crFlg, /*HANDLE hTempl,*/uint flags)
{
	SIntHandle fh = ::CreateFileW(pPath, mode, share, (SECURITY_ATTRIBUTES *)pSa, crMode, crFlg, 0/*template*/);
	if(!fh && ::GetLastError() == ERROR_ACCESS_DENIED && flags & (fileforcefACL|fileforcefMyACL|fileforcefAttr)) {
		if(flags & fileforcefACL) {
			ResetAcl(pPath, false);
			fh = ::CreateFileW(pPath, mode, share, (SECURITY_ATTRIBUTES *)pSa, crMode, crFlg, 0/*template*/);
		}
		if(!fh && ::GetLastError() == ERROR_ACCESS_DENIED && flags & (fileforcefMyACL|fileforcefAttr)) {
			if(flags & fileforcefMyACL) {
				ResetAcl(pPath, true);
				fh = ::CreateFileW(pPath, mode, share, (SECURITY_ATTRIBUTES *)pSa, crMode, crFlg, 0/*template*/);
			}
			if(!fh && ::GetLastError() == ERROR_ACCESS_DENIED && flags & (fileforcefAttr)) {
				if(flags & fileforcefAttr) {
					::SetFileAttributesW(pPath, FILE_ATTRIBUTE_NORMAL);
					fh = ::CreateFileW(pPath, mode, share, (SECURITY_ATTRIBUTES *)pSa, crMode, crFlg, 0/*template*/);
				}
			}
		}
	}
	return fh;
}