// GENCODE.CPP
// Copyright (c) A.Sobolev 2004


#if 0 // {
//
// @ModuleDecl(PPView@(vn))
//
struct @(vn)Filt {
};

struct @(vn)ViewItem {
};

class PPView@(vn) {
	SLAPI  PPView@(vn)();
	SLAPI ~PPView@(vn)();

	const  @(vn)Filt * SLAPI GetFilt() const;
	int    SLAPI EditFilt(@(vn)Filt *);

	int    SLAPI Init(const @(vn)Filt *);
	int    SLAPI InitIteration();
	int    SLAPI NextIteration(@(vn)ViewItem *);
	const  IterCounter & SLAPI GetIterCounter() const { return Counter; }

	int    SLAPI Browse(int modeless);
	int    SLAPI ViewTotal();
	int    SLAPI Print();
	//int    SLAPI Export();
private:
	DBQuery * SLAPI CreateBrowserQuery();
	//int    SLAPI CreateTempTable();

	IterCounter Counter;
	@(vn)Filt  Filt;
};

#endif // } 0

static int SLAPI SearchParamVal(const char * pParamStr, const char * pPar, char * pValBuf, size_t valBufLen)
{
	StringSet ss(';', pParamStr);
	char   temp_buf[128];
	for(uint p = 0; ss.get(&p, temp_buf, sizeof(temp_buf));) {
		char * p_eq = strchr(temp_buf, '=');
		if(p_eq) {
			*p_eq = 0;
			if(stricmp(strip(temp_buf), pPar) == 0) {
				strnzcpy(pValBuf, p_eq+1, valBufLen);
				return 1;
			}
		}
	}
	return 0;
}

static int SLAPI ProcessLineBuf(const char * pSrcBuf, char * pDestBuf, const char * pParamStr)
{
	size_t d = 0;
	for(size_t p = 0; pSrcBuf[p] != 0; p++) {
		int is_subst = 0;
		if(pSrcBuf[p] == '@' && pSrcBuf[p+1] == '(') {
			char par[32], val[128];
			for(size_t i = p+1, j = 0; pSrcBuf[i] != ')' && pSrcBuf[i] != 0; i++) {
				if(j < sizeof(par))
					par[j++] = pSrcBuf[i];
			}
			if(pSrcBuf[i] == 0)
				return 0;
			if(SearchParamVal(pParamStr, par, val, sizeof(val))) {
				d += strlen(strcpy(pDestBuf+d, val));
				p = i;
				is_subst = 1;
			}
		}
		if(!is_subst)
			pDestBuf[d++] = pSrcBuf[p];
	}
	return 1;
}

int SLAPI GenBySkeleton(const char * pSkelFile, const char * pOutFile, const char * pParamStr)
{
	int    ok = 1;
	uint   line_no = 0;
	char   err_str[512], line_buf[512], out_buf[1024];
	FILE * f_skel = 0;
	FILE * f_out  = 0;
	sprintf(err_str, "Error generation code: unable open skelton file '%s'\n", pSkelFile);
	THROW(f_skel = fopen(pSkelFile, "r"));
	sprintf(err_str, "Error generation code: unable open output file '%s'\n", pSkelFile);
	THROW(f_out = fopen(pOutFile, "a"));

	while(fgets(line_buf, sizeof(line_buf)-1, f_skel)) {
		line_no++;
		line_buf[sizeof(line_buf)-1] = 0;
		chomp(line_buf);
		sprintf(err_str, "Error generation code: unable priocess line %u\n", line_no);
		THROW(ProcessLineBuf(line_buf, out_buf, pParamStr));
		fprintf(f_out, "%s\n", out_buf);
	}
	CATCH
		ok = 0;
		printf(err_str);
	ENDCATCH
	if(f_skel)
		fclose(f_skel);
	if(f_out)
		fclose(f_out);
	return ok;
}

int SLAPI GenPPView(const char * pSkelFileName, const char * pOutFileName, const char * pViewNam)
{
	char param_str[128];
	sprintf(param_str, "vn=%s", pViewName);
	return GenBySkeleton(pSkelFileName, pOutFileName, param_str);
}

int main(int argc, char * argv[])
{
	if(argc < 4) {
		printf("Usage: gencode skel_file_name out_file_name, ppview_name\n");
		return -1;
	}
	return GenPPView(argv[1], argv[2], argv[3]) ? 0 : -1;
}

