#include "sierrachart.h"
#include "base data entries.h"
SCDLLName("BASE DATA ENTRIES")

/****************************************************************************************************************************************************************************************************************
hleda bar o urcite delce a deltu o urcite velikosti (meri pretlak). NERESI smer baru, pouze jeho velikost, RESI ale smer delty (ask vs. bid) - mohu tedy mit rostouci bar s negativni deltou a naopak
-vstupuje na close baru DO smeru aktivni strany ktera je chycena v delte
//****************************************************************************************************************************************************************************************************************/
SCSFExport scsf_delta_per_bar_break(SCStudyInterfaceRef sc)
{
	SCInputRef min_range_baru = sc.Input[0];
	SCInputRef max_range_baru = sc.Input[1];
	SCInputRef min_ask = sc.Input[2];
	SCInputRef max_ask = sc.Input[3];
	SCInputRef min_bid = sc.Input[4];
	SCInputRef max_bid = sc.Input[5];
	SCInputRef ATR_perioda = sc.Input[6];
	SCInputRef ATR_typ_prumeru = sc.Input[7];
	SCInputRef typ_pokynu = sc.Input[10];
	SCInputRef zacatek_rth = sc.Input[20];
	SCInputRef konec_rth = sc.Input[21];
	SCSubgraphRef ATR = sc.Subgraph[0];
	
	if (sc.SetDefaults)
	{
		//inputs
		min_range_baru.Name = "Bar range min";
		min_range_baru.SetInt(0);
		max_range_baru.Name = "Bar range max";
		max_range_baru.SetInt(10);

		min_bid.Name = "Bid min";
		min_bid.SetInt(200);
		max_bid.Name = "Bid max";
		max_bid.SetInt(9999);

		min_ask.Name = "Ask min";
		min_ask.SetInt(200);
		max_ask.Name = "Ask max";
		max_ask.SetInt(9999);

		ATR_perioda.Name = "ATR period";
		ATR_perioda.SetInt(7);
		ATR_typ_prumeru.Name = "ATR type";
		ATR_typ_prumeru.SetMovAvgType(0);

		typ_pokynu.Name = "Order type";
		typ_pokynu.SetCustomInputStrings("Obycejny pokyn;ATR pokyn;Range-Based pokyn");
		typ_pokynu.SetCustomInputIndex(0);
		
		zacatek_rth.Name = "Trade From:";
		zacatek_rth.SetTime(HMS_TIME(8, 30, 0));
		konec_rth.Name = "Trade Till:";
		konec_rth.SetTime(HMS_TIME(15, 10, 00));

		//visible subgraph
		ATR.Name = "atr";
		ATR.DrawStyle = DRAWSTYLE_LINE;

		sc.GraphName = "Delta per bar (break)";
		sc.AutoLoop = 1;
		sc.GraphRegion = 1;
		sc.FreeDLL = 1;
		
		return;
	}
	
	sc.AllowMultipleEntriesInSameDirection = true;
	sc.MaximumPositionAllowed = 1000;
	sc.SupportReversals = false;
	sc.SendOrdersToTradeService = false;
	sc.AllowOppositeEntryWithOpposingPositionOrOrders = true;
	sc.SupportAttachedOrdersForTrading = true;
	sc.CancelAllOrdersOnEntriesAndReversals = false;
	sc.AllowEntryWithWorkingOrders = false;
	sc.CancelAllWorkingOrdersOnExit = false;
	sc.AllowOnlyOneTradePerBar = true;
	sc.MaintainTradeStatisticsAndTradesData = true;

	//set perzist vars 
	int &internal_order_ID_long = sc.GetPersistentInt(1);
	int &internal_order_ID_short = sc.GetPersistentInt(2);
	int &vstupni_index = sc.GetPersistentInt(3);
	int &previous_qt_perzist = sc.GetPersistentInt(10);
	int &already_logged_perzist = sc.GetPersistentInt(11);

	//set order types 
	// ------------- ATR pokyn
	sc.ATR(sc.BaseDataIn, ATR, ATR_perioda.GetInt(), ATR_typ_prumeru.GetMovAvgType());
	float hodnota_atr = ATR[sc.Index];
	ATR[sc.Index] = hodnota_atr;
	s_SCNewOrder prikaz_ATR;
	prikaz_ATR.OrderQuantity = 1;
	prikaz_ATR.OrderType = SCT_ORDERTYPE_MARKET;
	prikaz_ATR.Target1Offset = hodnota_atr / sc.TickSize / 2.5;
	prikaz_ATR.StopAllOffset = hodnota_atr / sc.TickSize / 2.5;
	// ------------- range based pokyn
	float range_baru = abs(sc.High[sc.Index] - sc.Low[sc.Index]);
	float range_baru_v_ticich = range_baru / sc.TickSize;
	s_SCNewOrder pokyn_dle_range_baru;
	pokyn_dle_range_baru.OrderQuantity = 1;
	pokyn_dle_range_baru.OrderType = SCT_ORDERTYPE_MARKET;
	pokyn_dle_range_baru.Target1Offset = range_baru * 0.75;
	pokyn_dle_range_baru.StopAllOffset = range_baru * 0.75;
	//-------------- obyc pokyn
	s_SCNewOrder obycejny_pokyn;
	obycejny_pokyn.OrderQuantity = 1;
	obycejny_pokyn.OrderType = SCT_ORDERTYPE_MARKET;

	//set position details
	s_SCPositionData pozice;
	sc.GetTradePosition(pozice);
	

	if (is_rth(sc))
	{
		if (range_baru >= (min_range_baru.GetInt()*sc.TickSize) && range_baru <= (max_range_baru.GetInt()*sc.TickSize)
			&& (sc.BidVolume[sc.Index] - sc.AskVolume[sc.Index]) > min_bid.GetInt() 
			&& (sc.BidVolume[sc.Index] - sc.AskVolume[sc.Index]) < max_bid.GetInt() 
			&& sc.GetBarHasClosedStatus() == BHCS_BAR_HAS_CLOSED)
		{
			int entry_check = 0;
			if (typ_pokynu.GetIndex() == 0) { entry_check = (int)sc.SellEntry(obycejny_pokyn); }
			else if (typ_pokynu.GetIndex() == 1) { entry_check = (int)sc.SellEntry(prikaz_ATR); }
			else if (typ_pokynu.GetIndex() == 2) { entry_check = (int)sc.SellEntry(pokyn_dle_range_baru); }

			//log after the  ----- SHORT ENTRY --------- 

			if (entry_check > 0 && already_logged_perzist == 0)
			{				
				sc.AddMessageToLog(create_string_for_sierra_log(sc), 0); //calling custom function
				already_logged_perzist = 1;
				log_into_txt_file(sc); //calling custom function
			}
			if (pozice.PositionQuantity == 0) { already_logged_perzist = 0; } 
		}
		if (range_baru >= min_range_baru.GetInt()*sc.TickSize && range_baru <= max_range_baru.GetInt()*sc.TickSize
			&& (sc.AskVolume[sc.Index] - sc.BidVolume[sc.Index]) > min_ask.GetInt() 
			&& (sc.AskVolume[sc.Index] - sc.BidVolume[sc.Index]) < max_ask.GetInt()
			&& sc.GetBarHasClosedStatus() == BHCS_BAR_HAS_CLOSED)
		{
			int entry_check = 0;
			if (typ_pokynu.GetIndex() == 0) { entry_check = (int)sc.BuyEntry(obycejny_pokyn); }
			else if (typ_pokynu.GetIndex() == 1) { entry_check = (int)sc.BuyEntry(prikaz_ATR); }
			else if (typ_pokynu.GetIndex() == 2) { entry_check = (int)sc.BuyEntry(pokyn_dle_range_baru); }
			
			//log after the  ----- LONG ENTRY --------- 

			if (entry_check > 0 && already_logged_perzist == 0)
			{
				sc.AddMessageToLog(create_string_for_sierra_log(sc), 0); //calling custom function
				already_logged_perzist = 1;
				log_into_txt_file(sc); //calling custom function
			}
			if (pozice.PositionQuantity == 0) { already_logged_perzist = 0; } 
		}
	}

	if (sc.BaseDateTimeIn[sc.Index].GetTime() > konec_rth.GetTime() && pozice.PositionQuantity != 0) { sc.FlattenAndCancelAllOrders(); }
	
	//pripsat ze se ma zavrit soubor do kteryho zapisuju kdyz se zavre siera nebo tak neco
}

/****************************************************************************************************************************************************************************************************************
ok - funguje
uplne stejny jako predchozi ale vstupuje na druhou stranu. 
////****************************************************************************************************************************************************************************************************************/
//SCSFExport scsf_delta_per_bar_reverz(SCStudyInterfaceRef sc)
//{
//	SCSubgraphRef ATR = sc.Subgraph[0];
//
//	SCInputRef min_range_baru = sc.Input[1];
//	SCInputRef max_range_baru = sc.Input[2];
//
//	SCInputRef min_ask = sc.Input[9];
//	SCInputRef max_ask = sc.Input[10];
//	SCInputRef min_bid = sc.Input[11];
//	SCInputRef max_bid = sc.Input[12];
//
//	SCInputRef zacatek_rth = sc.Input[5];
//	SCInputRef konec_rth = sc.Input[6];
//
//	SCInputRef ATR_perioda = sc.Input[7];
//	SCInputRef ATR_typ_prumeru = sc.Input[8];
//
//	SCInputRef typ_pokynu = sc.Input[20];
//
//	if (sc.SetDefaults)
//	{
//		sc.GraphName = "delta v baru reverz (neresi smer baru), free dll = 0";
//		sc.AutoLoop = 1;
//		sc.GraphRegion = 1;
//		sc.UpdateAlways = 1;
//		sc.FreeDLL = 0;
//
//		min_range_baru.Name = "min range baru v ticich";
//		min_range_baru.SetInt(0);
//
//		max_range_baru.Name = "max range baru v ticich";
//		max_range_baru.SetInt(10);
//
//		//bid ask thresholdy
//		min_bid.Name = "min bid";
//		min_bid.SetInt(200);
//		max_bid.Name = "max bid";
//		max_bid.SetInt(9999999);
//
//		min_ask.Name = "min ask";
//		min_ask.SetInt(200);
//
//		max_ask.Name = "max ask";
//		max_ask.SetInt(9999999);
//
//		//perioda ATR pro target
//		ATR_perioda.Name = "atr: perioda";
//		ATR_perioda.SetInt(7);
//
//		ATR_typ_prumeru.Name = "atr: typ prumeru";
//		ATR_typ_prumeru.SetMovAvgType(0);
//
//		ATR.Name = "atr";
//		ATR.DrawStyle = DRAWSTYLE_LINE;
//
//		//nastaveni casu
//		zacatek_rth.Name = "Trade From:";
//		zacatek_rth.SetTime(HMS_TIME(8, 30, 0));
//
//		konec_rth.Name = "Trade Till:";
//		konec_rth.SetTime(HMS_TIME(15, 10, 00));
//
//		//nastaveni typu pokynu
//		typ_pokynu.Name = "typ pokynu";
//		typ_pokynu.SetCustomInputStrings("obycejny pokyn;atr pokyn;range-based pokyn");
//		typ_pokynu.SetCustomInputIndex(0);
//
//		return;
//	}
//
//	sc.AllowMultipleEntriesInSameDirection = true;
//	sc.MaximumPositionAllowed = 1000;
//	sc.SupportReversals = false;
//	sc.SendOrdersToTradeService = false;
//	sc.AllowOppositeEntryWithOpposingPositionOrOrders = true;
//	sc.SupportAttachedOrdersForTrading = true;
//	sc.CancelAllOrdersOnEntriesAndReversals = false;
//	sc.AllowEntryWithWorkingOrders = false;
//	sc.CancelAllWorkingOrdersOnExit = false;
//	sc.AllowOnlyOneTradePerBar = true;
//	sc.MaintainTradeStatisticsAndTradesData = true;
//
//	//============================================= nadefinovani pokynu ==========================================================//
//
//	//ATR pokyn
//	sc.ATR(sc.BaseDataIn, ATR, ATR_perioda.GetInt(), ATR_typ_prumeru.GetMovAvgType());
//	float hodnota_atr = ATR[sc.Index];
//	ATR[sc.Index] = hodnota_atr;
//	
//	s_SCNewOrder prikaz_ATR;
//	prikaz_ATR.OrderQuantity = 1;
//	prikaz_ATR.OrderType = SCT_ORDERTYPE_MARKET;
//	prikaz_ATR.Target1Offset = hodnota_atr / sc.TickSize / 2.5;
//	prikaz_ATR.StopAllOffset = hodnota_atr / sc.TickSize / 2.5;
//
//	//OCO pokyn
//	s_SCNewOrder prikaz_OCO;
//	prikaz_OCO.OrderQuantity = 2;
//	prikaz_OCO.OrderType = SCT_ORDERTYPE_MARKET;
//	prikaz_OCO.Target1Offset = 13 * sc.TickSize;
//	prikaz_OCO.Target2Offset = 25 * sc.TickSize;
//	prikaz_OCO.StopAllOffset = 13 * sc.TickSize;
//
//	//range based pokyn
//	float range_baru = abs(sc.High[sc.Index] - sc.Low[sc.Index]);
//	float range_baru_v_ticich = range_baru / sc.TickSize;
//
//	s_SCNewOrder pokyn_dle_range_baru;
//	pokyn_dle_range_baru.OrderQuantity = 1;
//	pokyn_dle_range_baru.OrderType = SCT_ORDERTYPE_MARKET;
//	pokyn_dle_range_baru.Target1Offset = range_baru * 0.75;
//	pokyn_dle_range_baru.StopAllOffset = range_baru * 0.75;
//
//	//obyc pokyn
//	s_SCNewOrder obycejny_pokyn;
//	obycejny_pokyn.OrderQuantity = 1;
//	obycejny_pokyn.OrderType = SCT_ORDERTYPE_MARKET;
//
//	//============================================= nadefinovani vstupni podminky ==========================================================//
//
//	int& internal_order_ID_long = sc.GetPersistentInt(1);
//	int& internal_order_ID_short = sc.GetPersistentInt(2);
//
//	int& vstupni_index = sc.GetPersistentInt(3);
//
//	int vstoupil = 0;
//
//	if (sc.BaseDateTimeIn[sc.Index].GetTime() > zacatek_rth.GetTime() && sc.BaseDateTimeIn[sc.Index].GetTime() < konec_rth.GetTime())
//	{
//		if (range_baru >= (min_range_baru.GetInt()*sc.TickSize) && range_baru <= (max_range_baru.GetInt()*sc.TickSize) //ma range v urcitym rozmezi ticku
//			&& (sc.BidVolume[sc.Index] - sc.AskVolume[sc.Index]) > min_bid.GetInt() && (sc.BidVolume[sc.Index] - sc.AskVolume[sc.Index]) < max_bid.GetInt() //ma negativni deltu v urcitym pasmu
//			&& sc.GetBarHasClosedStatus() == BHCS_BAR_HAS_CLOSED)
//		{
//			if (typ_pokynu.GetIndex() == 0) { vstoupil = (int)sc.BuyEntry(obycejny_pokyn); }
//			else if (typ_pokynu.GetIndex() == 1) { vstoupil = (int)sc.BuyEntry(prikaz_ATR); }
//			else if (typ_pokynu.GetIndex() == 2) { vstoupil = (int)sc.BuyEntry(pokyn_dle_range_baru); }
//
//			int& vstupni_index = sc.GetPersistentInt(8);
//			int& uz_vypsal = sc.GetPersistentInt(9);
//
//			uz_vypsal = 0;
//
//			//vypis do message logu po vstupu - nefunguje 100% optimalne, NEKDY vyloguje vicekrat jeden vstup .. nevim jak to odladit
//			if (uz_vypsal == 0)
//			{
//				s_SCPositionData pozice;
//				sc.GetTradePosition(pozice);
//				t_OrderQuantity32_64 velikost_pozice = pozice.PositionQuantity;
//				double vstupni_cena_long = pozice.AveragePrice;
//				double vstupni_cas_long = pozice.LastFillDateTime;
//
//				//prepocet casu na citelny format
//				int Year, Month, Day, Hour, Minute, Second;
//				DATETIME_TO_YMDHMS(vstupni_cas_long, Year, Month, Day, Hour, Minute, Second);
//
//				float bid_volume = -(sc.BidVolume[sc.Index] - sc.AskVolume[sc.Index]);
//				float pocet_asku = sc.BaseData[SC_ASKNT][sc.Index];
//				float pocet_bidu = sc.BaseData[SC_BIDNT][sc.Index];
//				float volume = sc.Volume[sc.Index];
//				float range_baru_v_ticich = (sc.High[sc.Index] - sc.Low[sc.Index]) / sc.TickSize;
//
//				std::string smer = "Long";
//
//				SCString vypis_parametru;
//				vypis_parametru.Format("Vstupni cena: %f | Datum a cas: %i-%i-%i  %i:%i:%i | Volume: %f | Ask/Bid Volume Difference Bars: %f | Number of Trades Ask: %f | Number of Trades Bid: %f | Range baru v ticich: %f | ATR exp. perioda 7: %f | Smer: %s", vstupni_cena_long, Year, Month, Day, Hour, Minute, Second, volume, bid_volume, pocet_asku, pocet_bidu, range_baru_v_ticich, hodnota_atr, smer);			
//				sc.AddMessageToLog(vypis_parametru, 0);
//
//				uz_vypsal = 1;
//
//				//vynuluj kdyz se index posune
//				if (vstupni_index != sc.CurrentIndex) { uz_vypsal = 0; }
//			}
//		}
//
//		if (range_baru >= min_range_baru.GetInt()*sc.TickSize && range_baru <= max_range_baru.GetInt()*sc.TickSize
//			&& (sc.AskVolume[sc.Index] - sc.BidVolume[sc.Index]) > min_ask.GetInt() && (sc.AskVolume[sc.Index] - sc.BidVolume[sc.Index]) < max_ask.GetInt()
//			&& sc.GetBarHasClosedStatus() == BHCS_BAR_HAS_CLOSED)
//
//		{
//			if (typ_pokynu.GetIndex() == 0) { vstoupil = (int)sc.SellEntry(obycejny_pokyn); }
//			else if (typ_pokynu.GetIndex() == 1) { vstoupil = (int)sc.SellEntry(prikaz_ATR); }
//			else if (typ_pokynu.GetIndex() == 2) { vstoupil = (int)sc.SellEntry(pokyn_dle_range_baru); }
//
//			int& uz_vypsal = sc.GetPersistentInt(10);
//			uz_vypsal = 0;
//
//			//vypis do logu po vstupu - nefunguje 100% optimalne, NEKDY vyloguje vicekrat jeden vstup .. nevim jak to odladit
//			if (uz_vypsal == 0)
//			{
//				s_SCPositionData pozice;
//				sc.GetTradePosition(pozice);
//				t_OrderQuantity32_64 velikost_pozice = pozice.PositionQuantity;
//				double vstupni_cena_short = pozice.AveragePrice;
//				double	vstupni_cas_short = pozice.LastFillDateTime;
//
//				std::string smer = "Short";
//
//				float ask_volume = (sc.AskVolume[sc.Index] - sc.BidVolume[sc.Index]);
//				float pocet_asku = sc.BaseData[SC_ASKNT][sc.Index];
//				float pocet_bidu = sc.BaseData[SC_BIDNT][sc.Index];
//				float volume = sc.Volume[sc.Index];
//				float range_baru_v_ticich = (sc.High[sc.Index] - sc.Low[sc.Index]) / sc.TickSize;
//
//				int Year, Month, Day, Hour, Minute, Second;
//				DATETIME_TO_YMDHMS(vstupni_cas_short, Year, Month, Day, Hour, Minute, Second);
//
//				SCString vypis_parametru;
//				vypis_parametru.Format("Vstupni cena: %f | Datum a cas: %i-%i-%i  %i:%i:%i | Volume: %f | Ask/Bid Volume Difference Bars: %f | Number of Trades Ask: %f | Number of Trades Bid: %f | Range baru v ticich: %f | ATR exp. perioda 7: %f | Smer: %s",
//					vstupni_cena_short, Year, Month, Day, Hour, Minute, Second, volume, ask_volume, pocet_asku, pocet_bidu, range_baru_v_ticich, hodnota_atr, smer);
//
//				sc.AddMessageToLog(vypis_parametru, 0);
//				uz_vypsal = 1;
//
//				//vynuluj kdyz se index posune
//				if (vstupni_index != sc.CurrentIndex) { uz_vypsal = 0; }
//			}
//		}
//	}
//
//	//============================================================= flat na konci dne ==============================================================================//
//
//	s_SCPositionData pozice;
//	sc.GetTradePosition(pozice);
//	t_OrderQuantity32_64 velikost_pozice = pozice.PositionQuantity;
//
//	if (sc.BaseDateTimeIn[sc.Index].GetTime() > konec_rth.GetTime() && velikost_pozice != 0) { sc.FlattenAndCancelAllOrders(); }
//
//}
//
