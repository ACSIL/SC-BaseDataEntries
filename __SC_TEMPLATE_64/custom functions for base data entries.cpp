#include "sierrachart.h"
#include "base data entries.h"

//logs (taking strings from other custom functions)
void log_into_txt_file(SCStudyInterfaceRef sc)
{
	std::string file_name{ create_txt_filename(sc) }; //calling custom function 
	std::ofstream out_file{};
	out_file.open(file_name, std::ofstream::app);
	if (out_file.is_open())
	{
		std::string test_logu{ create_string_for_sierra_log(sc) }; //calling custom function 
		out_file << test_logu << '\n';
	}
	else
	{
		SCString log_error{ "Error writing into file" };
		sc.AddMessageToLog(log_error, 1);
	}
	return;
}


//creates txt filename, in this filename it will write details of each entry. (not a generic foo as it  depends on the types of inputs). possible to rewrite using std::pair 
//use inputs with unsigned number only!!
std::string create_txt_filename(SCStudyInterfaceRef sc)
{
	n_ACSIL::s_BarPeriod bar_period;
	sc.GetBarPeriodParameters(bar_period);
	std::string tf{};
	switch (bar_period.IntradayChartBarPeriodType) //enum to string from this page https://www.sierrachart.com/index.php?page=doc/ACSIL_Members_Functions.html#scGetBarPeriodParameters 
	{
	case 0:
		tf = "SECS_PER_BAR";
		break;
	case 1:
		tf = "VOLUME_PER_BAR";
		break;
	case 2:
		tf = "NUM_TRADES_PER_BAR";
		break;
	case 3:
		tf = "RANGE_IN_TICKS_STANDARD";
		break;
	case 4:
		tf = "RANGE_IN_TICKS_NEWBAR_ON_RANGEMET";
		break;
	case 5:
		tf = "RANGE_IN_TICKS_TRUE";
		break;
	case 6:
		tf = "RANGE_IN_TICKS_FILL_GAPS";
		break;
	case 7:
		tf = "REVERSAL_IN_TICKS";
		break;
	case 8:
		tf = "RENKO_IN_TICKS";
		break;
	case 9:
		tf = "DELTA_VOLUME_PER_BAR";
		break;
	case 10:
		tf = "FLEX_RENKO_IN_TICKS";
		break;
	case 11:
		tf = "RANGE_IN_TICKS_OPEN_EQUAL_CLOSE";
		break;
	case 12:
		tf = "PRICE_CHANGES_PER_BAR";
		break;
	case 13:
		tf = "MONTHS_PER_BAR";
		break;
	case 14:
		tf = "POINT_AND_FIGURE";
		break;
	case 15:
		tf = "FLEX_RENKO_INV";
		break;
	case 16:
		tf = "ALIGNED_RENKO";
		break;
	case 17:
		tf = "RANGE_IN_TICKS_NEW_BAR_ON_RANGE_MET_OPEN_EQUALS_PRIOR_CLOSE";
		break;
	case 18:
		tf = "CUSTOM";
		break;
	default:
		break;
	}

	std::string graph_name = sc.GraphName;
	std::string input_0_name = sc.Input[0].Name;
	std::string input_0_value = std::to_string(sc.Input[0].GetInt());
	std::string input_1_name = sc.Input[1].Name;
	std::string input_1_value = std::to_string(sc.Input[1].GetInt());
	std::string input_2_name = sc.Input[2].Name;
	std::string input_2_value = std::to_string(sc.Input[2].GetInt());
	std::string input_3_name = sc.Input[3].Name;
	std::string input_3_value = std::to_string(sc.Input[3].GetInt());
	std::string input_4_name = sc.Input[4].Name;
	std::string input_4_value = std::to_string(sc.Input[4].GetInt());
	std::string input_5_name = sc.Input[5].Name;
	std::string input_5_value = std::to_string(sc.Input[5].GetInt());
	std::string input_6_name = sc.Input[6].Name;
	std::string input_6_value = std::to_string(sc.Input[6].GetInt());
	std::string input_7_name = sc.Input[7].Name;
	std::string input_7_value = std::to_string(sc.Input[7].GetMovAvgType());
	std::string tf_value = std::to_string(bar_period.IntradayChartBarPeriodParameter1);

	std::string test{ graph_name + " - " + input_0_name + "(" + input_0_value + ")" +
		", " + input_1_name + "(" + input_1_value + ")" +
		", " + input_2_name + "(" + input_2_value + ")" +
		", " + input_3_name + "(" + input_3_value + ")" +
		", " + input_4_name + "(" + input_4_value + ")" +
		", " + input_5_name + "(" + input_5_value + ")" +
		", " + input_6_name + "(" + input_6_value + ")" +
		", " + input_7_name + "(" + input_7_value + ")" +
		", " + tf + "(" + tf_value + ")" +
		".txt" };

	for (std::string::iterator p = test.begin(); test.end() != p; ++p) *p = toupper(*p); //capitalize the string (invokes undefined behavior when toupper is called with negative numbers)

	return test;
}

//makes string for internal log 
SCString create_string_for_sierra_log(SCStudyInterfaceRef sc)
{
	//set position details
	s_SCPositionData pozice;
	sc.GetTradePosition(pozice);
	//local vars
	double vstupni_cas_long = pozice.LastFillDateTime;
	double vstupni_cas_short = pozice.LastFillDateTime;
	double price = pozice.AveragePrice;
	int Year, Month, Day, Hour, Minute, Second;
	DATETIME_TO_YMDHMS(vstupni_cas_long, Year, Month, Day, Hour, Minute, Second);

	//set variables to be logged out
	float bid_volume = -(sc.BidVolume[sc.Index] - sc.AskVolume[sc.Index]);
	float ask_volume = (sc.AskVolume[sc.Index] - sc.BidVolume[sc.Index]);
	float pocet_asku = sc.BaseData[SC_ASKNT][sc.Index];
	float pocet_bidu = sc.BaseData[SC_BIDNT][sc.Index];
	float volume = sc.Volume[sc.Index];

	SCString vypis_parametru;
	vypis_parametru.Format("Price: %f, Datum a cas: %i-%i-%i  %0.2i:%0.2i:%0.2i | Volume: %f | Ask/Bid Volume Difference Bars: %f | Number of Trades Ask: %f | Number of Trades Bid: %f",
		price, Year, Month, Day, Hour, Minute, Second, volume, bid_volume, pocet_asku, pocet_bidu);

	return vypis_parametru;
}

bool is_rth(SCStudyInterfaceRef sc)
{
	if (sc.BaseDateTimeIn[sc.Index].GetTime() > sc.Input[20].GetTime() && sc.BaseDateTimeIn[sc.Index].GetTime() < sc.Input[21].GetTime())
		return 1;
	else
		return 0;
}