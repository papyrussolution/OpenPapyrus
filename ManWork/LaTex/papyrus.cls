% PAPYURS.CLS
% Copyright (c) A.Sobolev 2008, 2009, 2010, 2011, 2012, 2013, 2017
% Правила оформления документации на систему Papyrus
%
% Большая часть заимствуется из book.cls с необходимыми модификациями.
%
\newcommand\classname{papyrus}

\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{papyrus}[2008/05/31 Version 1.00 Papyrus LaTeX Document Class]
\LoadClass[a4paper,9pt,final]{book}
\usepackage{cmap} % Делает PDF-файл способным к копированию и поиску
\usepackage[cp1251]{inputenc}
\usepackage[TS1, T2A]{fontenc} % Улучшает качество шрифтов при выводе в PDF
\usepackage[english,russian]{babel}
\usepackage[pdftex]{graphicx}
\usepackage{wrapfig}
\usepackage{fancybox}
\usepackage{longtable}
\usepackage{hhline}
\usepackage[usenames]{color}
\usepackage{textcomp}
\usepackage{MnSymbol} % Специальные символы (используются в математической моде)
\usepackage{fancyvrb}
%
% Подключение пакета hyperref (с нюансами русских символов)
%
\usepackage{xcolor}
\usepackage[
	pdftex, 
	unicode, 
	linkbordercolor=white, 
	colorlinks=true,
	linkcolor=blue
]{hyperref}
% \usepackage{literat}
%
% Определение цветов
%
\definecolor{gray85}{gray}{0.85}
%
% Определение параметров страницы
%
\pagestyle{headings}
\pagenumbering{arabic}
\frenchspacing

\topmargin=-0.5in
\textwidth=5.7in
\textheight=10.0in
%
% Стили колонтитулов
%
%
% FancyHeader {
%
\usepackage{fancyhdr} \pagestyle{fancy}

\fancyhead{} % clear all header fields
\fancyhead[LE]{\small\mdseries\thepage}
\fancyhead[RE]{\small\mdseries\leftmark}
\fancyhead[LO]{\small\mdseries\rightmark}
\fancyhead[RO]{\small\mdseries\thepage}
\fancyfoot{} % clear all footer fields
\renewcommand{\headrulewidth}{0.4pt}
\renewcommand{\footrulewidth}{0.0pt}
%
% } End of FancyHeader
%
%
% Стили заголовков
%
\renewcommand \thechapter {\@arabic\c@chapter}%{\@Roman\c@chapter}
\renewcommand \thesection       {}%{\thechapter-\@arabic\c@section}
\renewcommand \thesubsection    {}%{\thesection.\@arabic\c@subsection}
\renewcommand \thesubsubsection {}%{\thesubsection .\@arabic\c@subsubsection}
\renewcommand \theparagraph     {}%{\thesubsubsection.\@arabic\c@paragraph}
\renewcommand \thesubparagraph  {}%{\theparagraph.\@arabic\c@subparagraph}

\renewcommand{\sectionmark}[1]{\markright{#1}}
%
% Эксперименты с пакетом titlesec
%
%\usepackage{titlesec}
%\titleformat{\section}{\color{green} \titlerule \vspace{.6ex} \color{black}\sffamily\huge\bfseries}{}{0.0em}{}
%\titlespacing{\section}{-0.5in}{3.5ex \@plus 1ex \@minus .2ex}{2.3ex \@plus .2ex}

\renewcommand\section{
	\@startsection{section}
	{1}                                % Уровень вложенности
	{-1.2in}%{0\p@}%{\z@}                % Отступ от левого поля //
	{-3.5ex \@plus -1ex \@minus -.2ex} % Вертикальный отступ перед заголовком
	{2.3ex \@plus .2ex}                % Вертикальный отступ после заголовка
	{\sffamily\huge\bfseries}          % Формат текста наименования раздела
}
\renewcommand\subsection{
	\@startsection{subsection}
	{2}
	{-0.75in}%{-0.23in}%{-\parindent}%{0\p@}%{\z@}
	{-3.25ex \@plus -1ex \@minus -.2ex}
	{1.5ex \@plus .2ex}
	{\sffamily\Large\bfseries}
}
\renewcommand\subsubsection{
	\@startsection{subsubsection}
	{3}
	{-\parindent}%{0\p@}%{\z@}
	{-3.25ex\@plus -1ex \@minus -.2ex}
	{1.5ex \@plus .2ex}
	{\sffamily\large\bfseries}
}
\renewcommand\paragraph{
	\@startsection{paragraph}
	{4}
	{\z@}
	{1ex} %{3.25ex \@plus 1ex \@minus .2ex}
	{-1ex} %{-1em}
	{\sffamily\normalsize\bfseries\slshape}
}
\renewcommand\subparagraph{
	\@startsection{subparagraph}
	{5}
	{\parindent}
	{3.25ex \@plus1ex \@minus .2ex}
	{-1em}
	{\sffamily\normalsize\mdseries}
}
%
% Переопределение стиля оформления элемента оглавления,
% соответствующего главе.
% Полностью заимствовано из book.cls с единственной модификацией:
% длина @tempdima изменена с 1.5em на 3.5em поскольку римские
% номера глав не вмещались и получалось не красиво.
%
\renewcommand*\l@chapter[2]{%
	\ifnum \c@tocdepth >\m@ne
		\addpenalty{-\@highpenalty}%
		\vskip 1.0em \@plus\p@
		\setlength\@tempdima{3.5em}% 1.5em
		\begingroup
			\parindent \z@ \rightskip \@pnumwidth
			\parfillskip -\@pnumwidth
			\leavevmode \bfseries
			\advance\leftskip\@tempdima
			\hskip -\leftskip
			#1\nobreak\hfil \nobreak\hb@xt@\@pnumwidth{\hss #2}\par
			\penalty\@highpenalty
		\endgroup
	\fi
}
%
% Переопределение стиля подписей под рисунками
% - Подписи должны быть выровнены по левому краю
% - Уменьшены вертикальные отбивки сверху и снизу
%
\setlength\abovecaptionskip{5\p@}
\setlength\belowcaptionskip{0\p@}

\renewcommand\@makecaption[2]{%
	\vskip\abovecaptionskip
	\sbox\@tempboxa{\itshape\footnotesize{#1. #2}}%
	%\ifdim \wd\@tempboxa >\hsize
		\itshape\footnotesize{#1. #2}\par
	%\else
	%	\global \@minipagefalse
	%	\hb@xt@\hsize{\hfil\box\@tempboxa\hfil}%
	%\fi	
	\vskip\belowcaptionskip
}
%
% Переопределение параметров списков
%
\renewcommand{\labelitemi}{$\filledmedsquare$}
\renewcommand{\labelitemii}{$\filledsquare$}
\renewcommand{\labelitemiii}{$\smallsquare$}

\renewcommand\@listi{
	\leftmargin\leftmargini
	\parsep  4\p@ \@plus2\p@ \@minus\p@
	\topsep  8\p@ \@plus2\p@ \@minus4\p@
	%\itemsep 4\p@ \@plus2\p@ \@minus\p@
	\itemsep 0\p@
}
\let\@listI\@listi
%\@listi
\renewcommand\@listii {
	\leftmargin\leftmarginii
	\labelwidth\leftmarginii
	\advance\labelwidth-\labelsep
	\topsep    4\p@ \@plus2\p@ \@minus\p@
	\parsep    2\p@ \@plus\p@  \@minus\p@
	%\itemsep   \parsep
}

\renewcommand\@listiii{
	\leftmargin\leftmarginiii
	\labelwidth\leftmarginiii
	\advance\labelwidth-\labelsep
	\topsep    2\p@ \@plus\p@\@minus\p@
	\parsep    \z@
	\partopsep \p@ \@plus\z@ \@minus\p@
	%\itemsep   \topsep
}

\renewcommand\@listiv {
	\leftmargin\leftmarginiv
	\labelwidth\leftmarginiv
	\advance\labelwidth-\labelsep
}

\renewcommand\@listv {
	\leftmargin\leftmarginv
	\labelwidth\leftmarginv
	\advance\labelwidth-\labelsep
}

\renewcommand\@listvi {\leftmargin\leftmarginvi
	\labelwidth\leftmarginvi
	\advance\labelwidth-\labelsep
}
%
% Переопределение текста в элементе description
%
\renewcommand*\descriptionlabel[1]{
	%\hspace\labelsep\normalfont\bfseries #1
	\hspace\labelsep\sffamily\bfseries\upshape\small #1
}
%
% Установка параметра пакета longtable, отвечающего за 
% размер обрабатываемого отрезка строк таблицы (по умолчанию 10)
%
\setcounter{LTchunksize}{200}
%
% Таблица, состоящая из двух колонок
%
\newenvironment{ppytable2}{
	%\begin{longtable}{ | p{0.27\textwidth} | p{0.67\textwidth} | } \hhline{==}} 
	%{\hhline{==}\end{longtable}}
	\begin{longtable}{ | p{0.27\textwidth} p{0.67\textwidth} | } \hline}
	{\hline \end{longtable}}	
	
\newenvironment{ppytable3}{
	\begin{longtable}{ | p{0.26\textwidth} | p{0.26\textwidth} | p{0.40\textwidth} | } \hhline{===}} 
	{\hhline{===}\end{longtable}}	

\newenvironment{ppytable-cda-act}{
	\begin{longtable}{ | p{0.18\textwidth} | p{0.07\textwidth} | p{0.67\textwidth} | } \hhline{===}} {\hhline{===} 

	\end{longtable}}
%
% Завершение строки таблицы
%
\newcommand\eor{\\[1ex] }%\hline}

\newcommand\ppypictpath{/PAPYRUS/ManWork/Pict/png}

\newcommand\ppypict[2]{
	\begin{figure}[ht]
		\includegraphics[scale=0.65]{\ppypictpath/#1}
		\caption{#2}\label{pic-#1}
	\end{figure}}

\newcommand\ppypictsc[3]{
	\begin{figure}[ht]
		\includegraphics[scale=#3]{\ppypictpath/#1}
		\caption{#2}\label{pic-#1}
	\end{figure}}

\newcommand\ppywrappict[2]{
	\begin{wrapfigure}{l}{0.6\textwidth}
		\includegraphics[scale=0.45]{\ppypictpath/#1}
		\caption{#2}\label{pic-#1}
	\end{wrapfigure}}

\newcommand\ppynotewidth[2]{
	\vskip 5\p@
	\noindent{
	\shadowbox{
		\begin{minipage}{#1\textwidth}
			\noindent{Замечание}
			\vskip 5\p@

			\noindent{#2}
		\end{minipage}
	}
	}
	\vskip 5\p@}

\newcommand\ppynote[1]{
	\ppynotewidth{0.94}{#1}}

\newcommand\ppyadminline[1]{
	\vskip 5\p@
	\noindent{\footnotesize #1}}

\newcommand\ppyadmin[1]{
	\vskip 5\p@
	\noindent{
	\shadowbox{
		\begin{minipage}{0.94\textwidth}
			{\footnotesize Администратору системы}
			\vskip 5\p@

			\noindent{\footnotesize #1}
		\end{minipage}
	}
	}
	\vskip 5\p@}
	
\newcommand\ppyresume[1]{
	\vskip 5\p@
	\noindent{
	\fbox{
		\begin{minipage}{0.94\textwidth}
			{\footnotesize Резюме}
			\vskip 5\p@

			\noindent{\footnotesize #1}
		\end{minipage}
	}
	}
	\vskip 5\p@}	
	
\newcommand\ppyexampletitlewidth[3]{
	\vskip 5\p@
	\colorbox[gray]{0.85}{
		\begin{minipage}{#2\textwidth}
			\textsl\small{#1}\hrule
			\vskip 5\p@
				
			\noindent\textsl\small{#3}
		\end{minipage}
	}
	\vskip 5\p@}

\newcommand\ppyexampletitle[2]{\ppyexampletitlewidth{#1}{0.965}{#2}}

\newcommand\ppyexample[1]{\ppyexampletitle{Пример}{#1}}

\newcommand\ppysrc[1]{
	\vskip 5\p@
	\ovalbox{
		\begin{minipage}{0.965\textwidth}
			{\noindent{\ttfamily\small #1}}
		\end{minipage}
	}
	\vskip 5\p@
}

\newcommand\ppysyntax[1]{{\rmfamily\itshape\small #1}}

\newcommand\dlgbutton[1]{
	%\ovalbox{\scriptsize #1}
	%\colorbox[gray]{0.2}{\textcolor{white}{\footnotesize #1}}
	{\footnotesize [#1]}}

\newcommand\dlgflag[1]{
	%\ovalbox{\scriptsize #1}
	%\colorbox[gray]{0.2}{\textcolor{white}{\footnotesize #1}}
	{\footnotesize $\checkmark$~#1}}
%
% Блок переключателей
%
\newcommand\dlgradioc[1]{{\footnotesize $\vdots$~#1}}
%
% Элемент блока переключателей
%
\newcommand\dlgradioi[1]{{\footnotesize $\odot$~#1}}

\newcommand\dlgcombo[1]{{\footnotesize $\medtriangledown$~#1}}

\newcommand\keyb[1]{\ovalbox{\scriptsize #1}}

\newcommand\rdir{$\rhd$}

\newcommand\ppymenu[1]{{\sffamily\upshape\footnotesize #1\index{#1}}}

\newcommand\ppybrand{\sffamily\upshape{Papyrus}}

\newcommand\trademark[1]{{\sffamily\small #1\texttrademark}}

\newcommand\qu[1]{\glqq #1\grqq}

\newcommand\ppyterm[1]{{\rmfamily\itshape\normalsize #1}}

\newcommand\ppyrsrv[1]{{\rmfamily\slshape\normalsize #1}}

%\newcommand\dltype[4]{
%	\paragraph{#1} [VisualBasic] #2. [C++] #3. 
%	\newline #4
%}

\renewcommand\indexname{Индекс}

\hyphenation {
	ин-стал-ля-ция
	ин-стал-ля-ции
}
