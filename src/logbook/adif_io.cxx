#include <FL/Fl.H>
#include <FL/filename.H>
#include <FL/fl_ask.H>


#include "adif_io.h"
#include "config.h"
#include "lgbook.h"

// These ADIF fields are a part of the QSO database
    
FIELD fields[] = {
//  TYPE,  NAME,    SIZE
    {ADDRESS,       "ADDRESS",      40},    // 0 - contacted stations mailing address
    {AGE,           "AGE",           3},    // 1 - contacted operators age in years
    {ARRL_SECT,     "ARRL_SECT",    12},    // 2 - contacted stations ARRL section
    {BAND,          "BAND",          6},    // 3 - QSO band
    {CALL,          "CALL",         10},    // 4 - contacted stations CALLSIGN
    {CNTY,          "CNTY",         20},    // 5 - secondary political subdivision, ie: STATE
    {COMMENT,       "COMMENT",      80},    // 6 - comment field for QSO
    {CONT,          "CONT",         10},    // 7 - contacted stations continent
    {CONTEST_ID,    "CONTEST_ID",    6},    // 8 - QSO contest identifier
    {COUNTRY,       "COUNTRY",      20},    // 9 - contacted stations DXCC entity name
    {CQZ,           "CQZ",           8},    // 10 - contacted stations CQ Zone
    {DXCC,          "DXCC",          8},    // 11 - contacted stations Country Code
    {FREQ, 			"FREQ",			10},    // 12 - QSO frequency in Mhz
    {GRIDSQUARE, 	"GRIDSQUARE",	 6},    // 13 - contacted stations Maidenhead Grid Square
    {MODE,			"MODE",          8},    // 14 - QSO mode
    {NAME, 			"NAME",         18},    // 15 - contacted operators NAME
    {NOTES, 		"NOTES",        80},    // 16 - QSO notes
    {QSLRDATE, 		"QSLRDATE",      8},    // 21 - QSL received date
    {QSLSDATE, 		"QSLSDATE",      8},    // 22 - QSL sent date
    {QSL_RCVD, 		"QSL_RCVD",      1},    // 23 - QSL received status
    {QSL_SENT, 		"QSL_SENT",      1},    // 24 - QSL sent status
    {QSO_DATE, 		"QSO_DATE",      8},    // 25 - QSO data
    {QTH, 			"QTH",          30},    // 27 - contacted stations city
    {RST_RCVD, 		"RST_RCVD",      3},    // 28 - received signal report
    {RST_SENT, 		"RST_SENT",      3},    // 29 - sent signal report
    {STATE, 		"STATE",         2},    // 34 - contacted stations STATE
    {STX, 			"STX",           8},    // 35 - QSO transmitted serial number
    {TIME_OFF, 		"TIME_OFF",      4},    // 37 - HHMM or HHMMSS in UTC
    {TIME_ON, 		"TIME_ON",       4},    // 38 - HHMM or HHMMSS in UTC
    {TX_PWR, 		"TX_PWR",        4},    // 39 - power transmitted by this station
// new fields
    {IOTA, 			"IOTA",       	 6},    // 13 
    {ITUZ,			"ITUZ",       	 6},    // 14 - ITU zone
    {OPERATOR,		"OPERATOR",   	10},    // 17 - Callsign of person loggin the QSO
    {PFX,			"PFX",        	 5},    // 18 - WPA prefix
    {PROP_MODE,		"PROP_MODE",  	 5},    // 19 - propogation mode
    {QSL_MSG,		"QSL_MSG",    	80},    // 20 - personal message to appear on qsl card
    {QSL_VIA, 		"QSL_VIA",    	30},    // 26
    {RX_PWR, 		"RX_PWR",     	 4},    // 30 - power of other station in watts
    {SAT_MODE,		"SAT_MODE",   	 8},    // 31 - satellite mode
    {SAT_NAME,		"SAT_NAME",   	12},    // 32 - satellite name
    {SRX,			"SRX",        	 5},    // 33 - received serial number for a contest QSO
    {TEN_TEN, 		"TEN_TEN",     	10},    // 36 - ten ten # of other station
    {VE_PROV,		"VE_PROV",       2},    // 40 - 2 letter abbreviation for Canadian Province
// fldigi specific fields
	{XCHG1,			"XCHG1",        20},    // 41 - contest exchange #1
	{XCHG2,			"XCHG2",        20},    // 41 - contest exchange #2
	{XCHG3,			"XCHG3",        20},    // 41 - contest exchange #3
    {EXPORT,		"EXPORT",        1}     // 41 - used to indicate record is to be exported
};

int fieldnbr (const char *s) {
    for (int i = 0; i < EXPORT; i++)
        if (strncasecmp( fields[i].name, s, fields[i].size) == 0)
            return i;
    return -1;
}

int findfield (char *p) {
    for (int i=0; i < EXPORT; i++)
        if (strncasecmp (p, fields[i].name, strlen(fields[i].name)) == 0)
            return i;
    if (strncasecmp (p, "EOR>", 4) == 0)
        return -1;
    return -2;
}

void cAdifIO::fillfield (int fieldnum, char *buff){
char *p = buff;
char numeral[8];
int n, fldsize;
    memset (numeral, 0, 8);
    n = 0;
    while (*p != ':' && n < 11) {p++; n++;}
    if (n == 11) return; // bad ADIF specifier ---> no ':' after field name
// found first ':'
    p++;
    n = 0;
    while (*p >= '0' && *p <= '9' && n < 8) {
        numeral[n++] = *p;
        p++;
    }
    fldsize = atoi(numeral);
    p = strchr(buff,'>'); // end of specifier +1 == > start of data
    if (!p) return;
    p++;
    char *flddata = new char[fldsize+1];
    memset (flddata, 0, fldsize + 1);
    strncpy (flddata, p, fldsize);
    adifqso.putField (fieldnum, (const char *)flddata); 
}

void cAdifIO::readFile (const char *fname, cQsoDb *db) {
    long filesize = 0;
    char *buff;
    int found;
// open the adif file
    adiFile = fopen (fname, "r");
    if (!adiFile)
        return;
// determine its size for buffer creation
    fseek (adiFile, 0, SEEK_END);
    filesize = ftell (adiFile);
    buff = new char[filesize + 1];
// read the entire file into the buffer
    fseek (adiFile, 0, SEEK_SET);
    fread (buff, filesize, 1, adiFile);
    fclose (adiFile);
    
    if (filesize == 0 || (strstr( buff, "<ADIF_VERS:")) == 0) {
    	fl_message("Not an ADIF log file");
    	return;
	}

	char *p1 = buff, *p2;
// is there a header?
    if (*p1 != '<') { // yes find the start of the records
        p1 = strchr(buff, '<');
        while (strncasecmp (p1+1,"EOH>", 4) != 0) {
            p1 = strchr(p1+1, '<'); // find next <> field
        }
        if (!p1) return;     // must not be an ADIF compliant file
        p1 += 1;
    }
    
    p2 = strchr(p1,'<'); // find first ADIF specifier
    adifqso.clearRec();
    while (p2) {
        found = findfield(p2+1); // -2 ==> not found; -1 <eor> 0 ...N field #
//        if (found == -2 ) return; // unknown field
        if (found > -1)
            fillfield (found, p2+1);
        else if (found == -1) { // <eor> reached; add this record to db
//update fields for older db
        	if (adifqso.getField(TIME_OFF)[0] == 0)
        		adifqso.putField(TIME_OFF, adifqso.getField(TIME_ON));
        	if (adifqso.getField(TIME_ON)[0] == 0)
        		adifqso.putField(TIME_ON, adifqso.getField(TIME_OFF));
            db->qsoNewRec (&adifqso);
            adifqso.clearRec();
        }
        p1 = p2 + 1;
        p2 = strchr(p1,'<');
    }
    db->SortByDate();
    return;

}

static const char *ADIFHEADER = "\
File: %s\n\
<ADIF_VERS:%d>%s\n\
<PROGRAMID:%d>%s\n\
<PROGRAMVERSION:%d>%s\n\
<EOH>\n";

static const char *adifmt = "<%s:%d>%s";

// write ALL or SELECTED records to the designated file


int cAdifIO::writeFile (const char *fname, cQsoDb *db) {
// open the adif file
    cQsoRec *rec;
    adiFile = fopen (fname, "w");
    if (!adiFile)
        return 1;
    fprintf (adiFile, ADIFHEADER,
             fl_filename_name(fname),
             strlen(ADIF_VERS), ADIF_VERS,
             strlen(PACKAGE_NAME), PACKAGE_NAME,
             strlen(PACKAGE_VERSION), PACKAGE_VERSION);
//    db->SortByDate();
    for (int i = 0; i < db->nbrRecs(); i++) {
        rec = db->getRec(i);
        if (rec->getField(EXPORT)[0] == 'E') {
			if (btnSelectQSOdate->value())
				fprintf (adiFile, adifmt, fields[QSO_DATE].name, 
						 strlen(rec->getField(QSO_DATE)), rec->getField(QSO_DATE));
			if (btnSelectTimeON->value())
				fprintf (adiFile, adifmt, fields[TIME_ON].name, 
						 strlen(rec->getField(TIME_ON)), rec->getField(TIME_ON));
			if (btnSelectTimeOFF->value())
				fprintf (adiFile, adifmt, fields[TIME_OFF].name, 
						 strlen(rec->getField(TIME_OFF)), rec->getField(TIME_OFF));
			if (btnSelectCall->value())
				fprintf (adiFile, adifmt, fields[CALL].name, 
						 strlen(rec->getField(CALL)), rec->getField(CALL));
			if (btnSelectName->value())
				fprintf (adiFile, adifmt, fields[NAME].name, 
						 strlen(rec->getField(NAME)), rec->getField(NAME));
			if (btnSelectBand->value())
				fprintf (adiFile, adifmt, fields[BAND].name, 
						 strlen(rec->getField(BAND)), rec->getField(BAND));
			if (btnSelectFreq->value())
				fprintf (adiFile, adifmt, fields[FREQ].name, 
						 strlen(rec->getField(FREQ)), rec->getField(FREQ));
			if (btnSelectMode->value())
				fprintf (adiFile, adifmt, fields[MODE].name, 
						 strlen(rec->getField(MODE)), rec->getField(MODE));
			if (btnSelectRSTsent->value())
				fprintf (adiFile, adifmt, fields[RST_SENT].name, 
						 strlen(rec->getField(RST_SENT)), rec->getField(RST_SENT));
			if (btnSelectRSTrcvd->value())
				fprintf (adiFile, adifmt, fields[RST_RCVD].name, 
						 strlen(rec->getField(RST_RCVD)), rec->getField(RST_RCVD));
			if (btnSelectQth->value())
				fprintf (adiFile, adifmt, fields[QTH].name, 
						 strlen(rec->getField(QTH)), rec->getField(QTH));
			if (btnSelectState->value())
				fprintf (adiFile, adifmt, fields[STATE].name, 
						 strlen(rec->getField(STATE)), rec->getField(STATE));
			if (btnSelectProvince->value())
				fprintf (adiFile, adifmt, fields[VE_PROV].name, 
						 strlen(rec->getField(VE_PROV)), rec->getField(VE_PROV));
			if (btnSelectCountry->value())
				fprintf (adiFile, adifmt, fields[COUNTRY].name, 
						 strlen(rec->getField(COUNTRY)), rec->getField(COUNTRY));
			if (btnSelectQSLrcvd->value())
				fprintf (adiFile, adifmt, fields[QSL_RCVD].name, 
						 strlen(rec->getField(QSL_RCVD)), rec->getField(QSL_RCVD));
			if (btnSelectQSLsent->value())
				fprintf (adiFile, adifmt, fields[QSL_SENT].name, 
						 strlen(rec->getField(QSL_SENT)), rec->getField(QSL_SENT));
			if (btnSelectComment->value())
				fprintf (adiFile, adifmt, fields[COMMENT].name, 
						 strlen(rec->getField(COMMENT)), rec->getField(COMMENT));
			if (btnSelectSerialIN->value())
				fprintf (adiFile, adifmt, fields[SRX].name, 
						 strlen(rec->getField(SRX)), rec->getField(SRX));
			if (btnSelectSerialOUT->value())
				fprintf (adiFile, adifmt, fields[STX].name, 
						 strlen(rec->getField(STX)), rec->getField(STX));
			if (btnSelectXchg1->value())
				fprintf (adiFile, adifmt, fields[XCHG1].name, 
						 strlen(rec->getField(XCHG1)), rec->getField(XCHG1));
			if (btnSelectXchg2->value())
				fprintf (adiFile, adifmt, fields[XCHG2].name, 
						 strlen(rec->getField(XCHG2)), rec->getField(XCHG2));
			if (btnSelectXchg3->value())
				fprintf (adiFile, adifmt, fields[XCHG3].name, 
						 strlen(rec->getField(XCHG3)), rec->getField(XCHG3));
        	
            rec->putField(EXPORT,"");
            db->qsoUpdRec(i, rec);
            fprintf(adiFile, "<EOR>\n");
        }
    }    
    fclose (adiFile);
    return 0;
}

// write ALL records to the common log

int cAdifIO::writeLog (const char *fname, cQsoDb *db) {
// open the adif file
    char *szFld;
    cQsoRec *rec;
    adiFile = fopen (fname, "w");
    if (!adiFile)
        return 1;
    fprintf (adiFile, ADIFHEADER,
             fl_filename_name(fname),
             strlen(ADIF_VERS), ADIF_VERS,
             strlen(PACKAGE_NAME), PACKAGE_NAME,
             strlen(PACKAGE_VERSION), PACKAGE_VERSION);
//    db->SortByDate();
    for (int i = 0; i < db->nbrRecs(); i++) {
        rec = db->getRec(i);
        for (int j = 0; j < EXPORT; j++) {
            szFld = rec->getField(j);
            if (strlen(szFld))
                fprintf(adiFile, adifmt,
                    fields[j].name, strlen(szFld), szFld);
        }
        db->qsoUpdRec(i, rec);
        fprintf(adiFile, "<EOR>\n");
    }
    fclose (adiFile);
    return 0;
}
