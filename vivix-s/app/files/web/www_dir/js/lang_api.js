
var LANG = {"US":"us","KR":"kr","CH":"ch","JP":"jp"};


function get_locale_data(locale) {
	var locale_data;
	switch(locale)
	{
		case LANG.KR:
			locale_data = krDomain;
			break;
		case LANG.US:
			locale_data = usDomain;
			break;
		case LANG.CH:
			locale_data = chDomain;
			break;
		default:
			locale_data = usDomain;
			break;

	}
	return locale_data;
}
var gt = new Gettext({ domain : 'locale', locale_data : get_locale_data(LANG.US)});

function _(msgid) { return gt.gettext(msgid); }

function change_locale(locale,data)
{
	if(gt != "")
		delete(gt);

	if(locale == "")
	{
		var l = $(data).find("config").find("locale").attr("country");
		gt = new Gettext({ domain : 'locale', locale_data : get_locale_data(l)});
		return l;
	}
	else
	{

		gt = new Gettext({ domain : 'locale', locale_data : get_locale_data(locale)});
	}

}

function get_config(successFunc, errorFunc)
{
	var data = new HttpRequestData();
	data.type = "get";
	data.url = "../xml/portable_config.xml?time" + new Date();
	data.cache="false";
	data.cmdtype = 0;
	data.count = 0;
	sendHttpRequest(data, 
			function(data)
			{
			successFunc(data);
			},
			function(data)
			{
			errorFunc(data);
			});
}

