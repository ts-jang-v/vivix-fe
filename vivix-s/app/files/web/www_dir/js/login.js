var CMD_LOGIN = 0x01;
var CMD_LOGOUT = 0x04;
var CMD_CHANGE_LANGUAGE = 0x12;
/*
\brief : ajax 통신에 사용할 구조체
\param : none
\return: none
\note  :
*/
function HttpRequestData(){
	var url="";
	var dataType="";
	var type="";
	var count="";
	var userdata="";
}

/*
\brief : ajax 통신 리턴 값 처리 
\param : data - 응답 xml 
\return: 
\note  :

*/
function checkReturnValue(data, value)
{
	return $(data).find("RESPONSE").find(value).text();
}



/*
\brief : 
\param :
\return:
\note  :
*/
function login(id, passwd, successFunc, errorFunc)
{
	var data = new HttpRequestData();
	data.type = "post";
	data.url = "./cgi-bin/vieworks.cgi";
	data.cache="false";
	data.cmdtype = CMD_LOGIN;
	data.count = 2;
	data.userdata = id + "||" + passwd;

	sendHttpRequest(data, successFunc, errorFunc);

	delete(data);
}

/*
\brief : 
\param :
\return:
\note  :
*/
function logoutSess()
{
	logout(function(){}, function(){});
}


/*
\brief : 
\param :
\return:
\note  :
*/
function logout(successFunc, errorFunc)
{
	var data = new HttpRequestData();
	data.type = "post";
	data.url = "./cgi-bin/vieworks.cgi";
	data.cache="false";
	data.cmdtype = CMD_LOGOUT;
	data.count = 0;
	data.userdata = "";

	sendHttpRequest(data, successFunc, errorFunc);

	delete(data);
}


/*
\brief : ajax통신
\param : data - HttpRequsetData구조체, successFunc - success 콜백, errorFunc - error 콜백
\return: none
\note  :
*/
function sendHttpRequest(data, successFunc, errorFunc)
{
	var token = "||"
	var params =  "TYPE=" + data.cmdtype + "&CNT=" + data.count + "&PARAMS=" + data.userdata;
	$.ajax({
		type: data.type,
		dataType: data.dataType,
		cache: data.cache,
		url: data.url,
		data: params,
		success: function (data){
			successFunc(data);

		},
		error: function(data){
			errorFunc(data);

		}
	});

}



/*
\brief : 
\param :
\return:
\note  :
*/
function changeLanguage(country,successFunc, errorFunc)
{
	var data = new HttpRequestData();
	data.type = "post";
	data.url = "./cgi-bin/vieworks.cgi";
	data.cache="false";
	data.cmdtype = CMD_CHANGE_LANGUAGE;
	data.count = 1;
	data.userdata = country;

	sendHttpRequest(data, successFunc, errorFunc);

	delete(data);
}



