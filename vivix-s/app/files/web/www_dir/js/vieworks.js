// typedef
var CMD_LOGIN = 0x01;
var CMD_CHANGE_PASSWD = 0x02;
var CMD_SEND_PATIENT_INFORMATION = 0x03;
var CMD_LOGOUT = 0x04;
var CMD_CHECK_SESSION = 0x05;
var CMD_MODIFY_CONFIG = 0x06;
var CMD_ADD_PATIENT = 0x08;
var CMD_ADD_STEP = 0x09;
var CMD_JOIN_DETECTOR = 0x0d;
var CMD_DEL_PATIENT_INFO = 0x0e;
var CMD_INIT_PASSWORD = 0x0f;
var CMD_DEL_ONE_STUDY = 0x10;
var CMD_UPDATE_SESSION_TIMEOUT = 0x11;
var CMD_INVERT_IMAGE = 0x13;
var CMD_USE_FOR_HUMAN = 0x14;

// global variable
var g_currentPatient="";
var g_patientList="";
var g_stepList="";
var g_favoriteStepList="";
var g_extendedStepList="";

var g_EditBoxflag = false;

// API
/*
\brief : 
\param :
\return:
\note  :
*/

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
		timeout: 3000,
		success: function (data){
			successFunc(data);

		},
		error: function(data){
			errorFunc(data);

		}
	});

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
	data.url = "./cgi-bin/vieworks.cgi?time="+new Date();
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
function logout(successFunc, errorFunc)
{
	var data = new HttpRequestData();
	data.type = "post";
	data.url = "./cgi-bin/vieworks.cgi?time="+new Date();
	data.cache="false";
	data.cmdtype = CMD_LOGOUT;
	data.count = 0;
	data.userdata = "";

	sendHttpRequest(data, successFunc, errorFunc);

	delete(data);
}



/*
\brief : 
\param :
\return:
\note  :
*/
function changePasswd(user, passwd, newpasswd, successFunc, errorFunc)
{
	var data = new HttpRequestData();
	data.type = "post";
	data.url = "./cgi-bin/vieworks.cgi?time="+new Date();
	data.cache="false";
	data.cmdtype = CMD_CHANGE_PASSWD;
	data.count = 3;
	data.userdata = user + "||" + passwd + "||" + newpasswd;

	sendHttpRequest(data, successFunc, errorFunc);

	delete(data);
}


function getCurrentPatient(successFunc, errorFunc)
{
	var data = new HttpRequestData();
	data.type = "get";
	data.url = "./xml/current.xml?time="+new Date();
	data.cache="false";
	data.cmdtype = 0;
	data.count = 0;

	sendHttpRequest(data, successFunc, errorFunc);

	delete(data);
}


function setPatientInfo(id, name, accno,birth, study)
{
	var data = $(g_currentPatient).find("patient").find("info");
	var list = $(g_patientList).find("list").find("patient");
	$(list).each(function(){
			if(id == $(this).attr("id") && 
				name == $(this).attr("name") && 
				accno == $(this).attr("accno") && 
				study == ($(this).attr("studydate") + "/" +  $(this).attr("studytime")))
			{
				data.attr("name", $(this).attr("name"));
				data.attr("birth", $(this).attr("birth"));
				data.attr("id", $(this).attr("id"));
				data.attr("accno", $(this).attr("accno"));
				data.attr("studydate", $(this).attr("studydate"));
				data.attr("studytime", $(this).attr("studytime"));
				data.attr("reserve", $(this).attr("reserve"));
				
				var neutered = $(this).attr("neutered");
				if( neutered !="" && typeof(neutered) != "undefined" )
				{
					data.attr("neutered", neutered);
				}
				else
				{
					data.attr("neutered", _("NONE"));
				}
				var species =  $(this).attr("species_description");
				if(species!="" && typeof(species)!="undefined")
				{
					data.attr("species_description", species);
				}
				else
				{
					data.attr("species_description", _("NONE"));
				}
				var breed = $(this).attr("breed_description");
				if(breed!="" && typeof(breed)!="undefined")
				{
					data.attr("breed_description", breed);
				}
				else
				{
					data.attr("breed_description", _("NONE"));
				}

				var person =  $(this).attr("responsible_person");
				if(person!="" && typeof(person)!="undefined")
				{
					data.attr("responsible_person", person);
				}
				else
				{
					data.attr("responsible_person", _("NONE"));
				}
			}
			});

	/*
	*/
}

function setStepInfo(key, serial)
{
	var data = $(g_currentPatient).find("patient").find("step");
	var viewerserial = serial;
	if((typeof(serial) == "undefined") || (serial == "undefined") || (serial == undefined) )
	{
		viewerserial = $(g_stepList).find("step").find("viewer").attr("serial");
		if((typeof(viewerserial) == "undefined") || (viewerserial == "undefined") || (viewerserial == undefined) )
		{
			viewerserial = "0"
		}
	}
	data.attr("key", key);
	data.attr("viewerserial", viewerserial);
	if(key=="" || key == "0")
		return;
	var step = getStepInfoByKey(g_stepList,key); 
	data.attr("bodypart", step[0]);
	data.attr("projection", step[1]);
}

function setEmergencyPatient(id, name, studydate, studytime, birth)
{
	var data = $(g_currentPatient).find("patient").find("info");
	var viewerserial = $(g_stepList).find("step").find("viewer").attr("serial");
	if((typeof(viewerserial) == "undefined") || (viewerserial == "undefined") || (viewerserial == undefined) )
	{
		viewerserial = "0"
	}
	data.attr("name", name);
	data.attr("birth", birth);
	data.attr("id", id);
	data.attr("studydate", studydate);
	data.attr("studytime", studytime);
	data.attr("accno", "");
	data.attr("reserve", "");
	data.attr("species_description", "");
	data.attr("breed_description", "");
	data.attr("responsible_person", "");
	data.attr("neutered", "");

	data = $(g_currentPatient).find("patient").find("step");
	data.attr("key", "0");
	data.attr("bodypart", "none");
	data.attr("projection", "none");
	data.attr("viewerserial", viewerserial);
}

function getPatientListFromDetector(successFunc)
{
	var data = new HttpRequestData();
	data.type = "get";
	data.url = "./xml/patient.xml?time=" + new Date();
	data.cache="false";
	data.cmdtype = 0;
	data.count = 0;

	sendHttpRequest(data, 
			function(data){
				g_patientList = data;
				successFunc(data);
			}
			, function(data){
				deleteAllPatientInfo();
				sleep(1000); // 
				window.location = "login.html";
				alert(_("RE_LOGIN"));
			});

	delete(data);
}



function getStepFromDetector()
{
	var data = new HttpRequestData();
	data.type = "get";
	data.url = "./xml/step.xml?time=" + new Date();;
	data.cache="false";
	data.cmdtype = 0;
	data.count = 0;

	sendHttpRequest(data, 
			function(data){
				g_stepList = data;
				seperateStep(g_stepList);
			}
			, function(data){


			});

	delete(data);
}

function seperateStep(data)
{
	var list;
	var domParser1 = new DOMParser();
	var domParser2 = new DOMParser();
	var serial = $(data).find("step").find("viewer").attr("serial");
	g_favoriteStepList = domParser1.parseFromString("<?xml version=\"1.0\"?><step></step>", "text/xml");
	g_extendedStepList = domParser2.parseFromString("<?xml version=\"1.0\"?><step></step>", "text/xml");
	$(data).find("step").find("bodypart").each(function(){
			$(this).find("projection").each(function(){
				var key = $(this).attr("stepkey");
				var step = getStepInfoByKey(g_stepList,key);
				if($(this).attr("favorite") == "1")
				{
					list = g_favoriteStepList;
					addPatientStepList(list, step[0], step[1], key, serial);
				}
				list = g_extendedStepList;
				addPatientStepList(list, step[0], step[1], key, serial);
			});

		});
	delete(domParser1);
	delete(domParser2);
//	printDOMTree(g_favoriteStepList);
//	printDOMTree(g_extendedStepList);
}

function getStepInfoByKey(step, key)
{
	var info =["",""];
	var flag = false;

	$(step).find("step").find("bodypart").each(function(){

			var bodypart = $(this);
			$(bodypart).find("projection").each(function(){

				var projection = $(this);
				if(projection.attr("stepkey") == key)
				{
					info[1] = projection.attr("meaning");
					flag = true;
				}

				});

			if(flag==true)
			{
					info[0] = bodypart.attr("meaning");
					flag=false;
			}
			});

	return info;
}

function getPatientStepInfo(id, name, accno,birth, study)
{
	var list = $(g_patientList).find("list");
	var domParser = new DOMParser();
	var stepList = domParser.parseFromString("<?xml version=\"1.0\"?><step></step>", "text/xml");
	$(list).find("patient").each(function(){
			var patient = $(this);
			if(id == $(patient).attr("id") && 
				name == $(patient).attr("name") && 
				accno == $(patient).attr("accno") && 
				birth == $(patient).attr("birth") && 
				study == ($(patient).attr("studydate") + "/" +  $(patient).attr("studytime")))
			{
				$(patient).find("step").each(function(){
					var key = $(this).attr("key");
					var serial = $(this).attr("viewerserial");
					var step = getStepInfoByKey(g_stepList,key);
					addPatientStepList(stepList, step[0], step[1], key, serial);
					});

			}
			
			});
	delete(domParser);
	return stepList;
}

function addPatientStepList(list, bodypart, projection, key, serial)
{
	var step = $(list).find("step");
	var insert = false;
	var newprojection = document.createElement("projection");
	$(newprojection).attr("meaning",projection);
	$(newprojection).attr("stepkey",key);
	$(newprojection).attr("viewerserial",serial);

	$(step).find("bodypart").each(function(){
			if($(this).attr("meaning") == bodypart)
			{
				$(this).append($(newprojection));
				insert = true;
			}


			});

	if(insert == false)
	{
		var newbodypart = document.createElement("bodypart");
		$(newbodypart).attr("meaning",bodypart);
		$(step).append($(newbodypart));
		$(newbodypart).append($(newprojection));
	}

}

function sleep(msecs) 
{
	var start = new Date().getTime();
	var cur = start;
	while (cur - start < msecs) 
	{
		cur = new Date().getTime();
	}
}


function sendInformation(successFunc, errorFunc)
{
	var serial = $(g_stepList).find("step").find("viewer").attr("serial");
	var patient = $(g_currentPatient).find("patient").find("info");
	var step = $(g_currentPatient).find("patient").find("step");
	var data = new HttpRequestData();
	data.type = "post";
	data.url = "./cgi-bin/vieworks.cgi?time=" + new Date();
	data.cache="false";
	data.cmdtype = CMD_SEND_PATIENT_INFORMATION;
	data.count = 9;
	data.userdata = patient.attr("id") + "||" 
		+ patient.attr("accno") + "||"
		+ patient.attr("birth") + "||"
		+ patient.attr("name")  + "||"
		+ patient.attr("studydate")  + "||"
		+ patient.attr("studytime")  + "||"
		+ patient.attr("reserve") + "||"
		+ patient.attr("species_description") + "||"
		+ patient.attr("breed_description") + "||"
		+ patient.attr("responsible_person") + "||"
		+ patient.attr("neutered") + "||"
		+ step.attr("viewerserial") + "||"
		+ step.attr("key");

	sendHttpRequest(data, 
			function(data){
			successFunc(data);
			}
			, function(data){
			errorFunc(data);
			});

	delete(data);

}

function view(id)  {
	var viewElement=document.getElementById("loading_icon");
	var dimElement=document.getElementById(id);

	var elementWidth=parseInt(viewElement.style.width);
	var elementHeight=parseInt(viewElement.style.height);

	var windowX=(document.body.clientWidth-elementWidth)/2;
	var windowY=(document.body.clientHeight-elementHeight)/2;

	viewElement.style.left=windowX;
	viewElement.style.top=windowY;

	dimElement.style.visibility="visible";
}


function hide(id)  {
	var dimElement=document.getElementById(id);
	dimElement.style.visibility="hidden";
}

function addPatient(name, birth, id, accno, reserve, studydate, studytime, species, breed, responsible, neutered, successFunc, errorFunc)
{
	var list = $(g_patientList).find("list");
//	var exist = false;
//	$(list).find("patient").each(function(){
//			if($(this).attr("id") == id)
//				exist = true;
//			});

//	if(exist == false)
	{
		var newpatient = document.createElement("patient");
		$(newpatient).attr("id",id);
		$(newpatient).attr("accno",accno);
		$(newpatient).attr("name",name);
		$(newpatient).attr("birth",birth);
		$(newpatient).attr("studydate",studydate);
		$(newpatient).attr("studytime",studytime);
		$(newpatient).attr("reserve",reserve);
		$(newpatient).attr("species_description",species);
		$(newpatient).attr("breed_description",breed);
		$(newpatient).attr("responsible_person",responsible);
		$(newpatient).attr("neutered",neutered);
		$(newpatient).attr("enable",true);

		$(list).append($(newpatient));

		var data = new HttpRequestData();
		data.type = "post";
		data.url = "./cgi-bin/vieworks.cgi?time="+new Date();
		data.cache="false";
		data.cmdtype = CMD_ADD_PATIENT;
		data.count = 5;
		data.userdata = name + "||" + id + "||" + (accno == "" ? "none" : accno) + "||" +studydate + "||" + studytime + "||" +  (birth == "" ? "none" : birth) + "||" + (reserve == "" ? "none" : reserve) + "||" + (species == "" ? "none" : species) + "||" + (breed == "" ? "none" : breed) + "||" + (responsible == "" ? "none" : responsible) + "||" + (neutered == "" ? "none" : neutered);

		sendHttpRequest(data, successFunc, errorFunc);

		delete(data);

	}


}


function getDate() {
	var d = new Date();

	var s =
		leadingZeros(d.getFullYear(), 4) + 
		leadingZeros(d.getMonth() + 1, 2) + 
		leadingZeros(d.getDate(), 2); 

	delete(d);
	return s;
}


function getTime() {
	var d = new Date();

	var s =

		leadingZeros(d.getHours(), 2) + 
		leadingZeros(d.getMinutes(), 2) + 
		leadingZeros(d.getSeconds(), 2);

	delete(d);
	return s;
}


function leadingZeros(n, digits) {
	var zero = '';
	n = n.toString();

	if (n.length < digits) {
		for (i = 0; i < digits - n.length; i++)
			zero += '0';
	}
	return zero + n;
}



function addStepToPatient(id,name, accno, birth,studydate, studytime, key)
{
	var serial = $(g_stepList).find("step").find("viewer").attr("serial");
	var list = $(g_patientList).find("list");

	if(typeof(serial) == "undefined")
		serial = "0";

	$(list).find("patient").each(function(){
			if(id == $(this).attr("id") && 
				name == $(this).attr("name") && 
				accno == $(this).attr("accno") && 
				birth == $(this).attr("birth") && 
				studydate == $(this).attr("studydate")  && 
				studytime == $(this).attr("studytime")    )
			{
				var newstep = document.createElement("step");
				$(newstep).attr("key",key);
				$(newstep).attr("enable",true);

				$(this).append($(newstep));

				var data = new HttpRequestData();
				data.type = "post";
				data.url = "./cgi-bin/vieworks.cgi?time="+new Date();
				data.cache="false";
				data.cmdtype = CMD_ADD_STEP;
				data.count = 2;
				data.userdata = name + "||" + id+ "||" + (accno == "" ? "none" : accno)  + "||" +   (birth == "" ? "none" : birth) + "||" +  studydate + "||" + studytime + "||"  + key + "||" + serial;

				sendHttpRequest(data, function(){}, function(){});

				}
			});

}


function markExpEnd(patient)
{
	var id = $(patient).find("patient").find("info").attr("id");
	var name = $(patient).find("patient").find("info").attr("name");
	var accno = $(patient).find("patient").find("info").attr("accno");
	var birth = $(patient).find("patient").find("info").attr("birth");
	var study = $(patient).find("patient").find("info").attr("studydate") + "/" + $(patient).find("patient").find("info").attr("studytime");
	var key = $(patient).find("patient").find("step").attr("key");
	var list = $(g_patientList).find("list");
	var enable = false;
	$(list).find("patient").each(function(){
			if(id == $(this).attr("id") && 
				name == $(this).attr("name") && 
				accno == $(this).attr("accno") && 
				birth == $(this).attr("birth") && 
				study == ($(this).attr("studydate") + "/" +  $(this).attr("studytime")))
			{
				$(this).find("step").each(function(){
					if($(this).attr("key") == key)
						$(this).attr("enable",false);

					if($(this).attr("enable") == "true")
						enable = true;

					});

				if(enable == false)
					$(this).attr("enable",false);

			}

			});
}

function checkExpEndBIDnKey(id, name, accno, birth,study,key)
{
	
	var list = $(g_patientList).find("list");
	var ret = "true";
	$(list).find("patient").each(function(){
			if(id == $(this).attr("id") && 
				name == $(this).attr("name") && 
				accno == $(this).attr("accno") && 
				birth == $(this).attr("birth") && 
				study == ($(this).attr("studydate") + "/" +  $(this).attr("studytime")))
			{
				$(this).find("step").each(function(){
					if($(this).attr("key") == key)
					{
						ret = $(this).attr("enable");
						}

					});
				}
			});

	return ret;

}


function checkSession(successFunc, errorFunc)
{
	var data = new HttpRequestData();
	data.type = "post";
	data.url = "./cgi-bin/vieworks.cgi";
	data.cache="false";
	data.cmdtype = CMD_CHECK_SESSION;
	data.count = 0;
	data.userdata = "";

	sendHttpRequest(data, successFunc, errorFunc);

	delete(data);

}

function updateTimeout(successFunc, errorFunc)
{
	var data = new HttpRequestData();
	data.type = "post";
	data.url = "./cgi-bin/vieworks.cgi";
	data.cache="false";
	data.cmdtype = CMD_UPDATE_SESSION_TIMEOUT;
	data.count = 0;
	data.userdata = "";

	sendHttpRequest(data, successFunc, errorFunc);

	delete(data);

}


function joinDetector()
{
	var data = new HttpRequestData();
	data.type = "post";
	data.url = "./cgi-bin/vieworks.cgi?time="+new Date();
	data.cache="false";
	data.cmdtype = CMD_JOIN_DETECTOR;
	data.count = 0;
	data.userdata = "";

	sendHttpRequest(data, 
			function(data){
			}
			, function(data){
			});

	delete(data);
}


function deleteAllPatientInfo()
{

	var data = new HttpRequestData();
	data.type = "post";
	data.url = "./cgi-bin/vieworks.cgi?time="+new Date();
	data.cache="false";
	data.cmdtype = CMD_DEL_PATIENT_INFO;
	data.count = 0;
	data.userdata = "";

	sendHttpRequest(data, 
			function(data){
			}
			, function(data){
			});

	delete(data);


}


function checkInputData(item) {
	if(g_EditBoxflag == true)
		return;
	g_EditBoxflag = true;
	var onvalue = item.value;
	var findRestricChar = false;
	var retValue = "";
	//var token = /[`~!@#\':$%^&*\[\{\]\}\\|;:\'|\",<.>/?]/gi;
	var token = "`~!@#\':$%^&*\[\{\]\}\\|;:\'|\",<.>/?+-";

	for (i=0; i < onvalue.length;i++){
		ls_one_char = onvalue.charAt(i);
		for(j=0 ; j < token.length ; j++)
		{
			if(ls_one_char == token[j])
			{
				findRestricChar = true;
				break;
			}
		}
		
		if(j == token.length)
			retValue += ls_one_char;
	}

	if(findRestricChar == true)
	{
		alert(_("RESTRICTION_CHARACTER")); 
		item.value = retValue;
		item.focus();
	}
	g_EditBoxflag = false;


	/*
	var token = "[0-9|a-z|A-Z|=|_|+|(|)]";
	if(item.id == "menu2_2_name")
		token = "[0-9|a-z|A-Z|=|_|+|(|)| |]";
	count=0;
	for (i=0;i<onvalue.length;i++){
		ls_one_char = onvalue.charAt(i);
		if(ls_one_char.search(token) == -1){
			count++;
		}
	}
	if(count!=0) {
		alert(_("RESTRICTION_CHARACTER")); 
		item.value = "";
		item.focus();
		return false;
	}
	else {
		return false;
	}
	*/
}

function checkDate(event){ 
	var time = new Date() 
		, y = String(time.getFullYear()) 
		, m = time.getMonth() 
		, d = time.getDate() 
		, h = '-' 
		, lists = { 
keyup : [ 
			// 숫자, - 외 제거 
			[/[^\d\-]/, ''] 
			// 0000 > 00-00 
			, [/^(\d\d)(\d\d)$/, '$1-$2'] 
			// 00-000 > 00-00-0 
			, [/^(\d\d\-\d\d)(\d+)$/, '$1-$2'] 
			// 00-00-000 > 0000-00-0 
			, [/^(\d\d)-(\d\d)-(\d\d)(\d+)$/, '$1$2-$3-$4'] 
			// 00-00-0-0 > 0000-0-0 
			, [/^(\d\d)-(\d\d)-(\d\d?)(-\d+)$/, '$1$2-$3$4'] 
			// 0000-0000 > 0000-00-00 
			, [/^(\d{4}-\d\d)(\d\d)$/, '$1-$2'] 
			// 00000000 > 0000-00-00 
			, [/^(\d{4})(\d\d)(\d\d)$/, '$1-$2-$3'] 
			// 이탈 제거 
			, [/(\d{4}-\d\d?-\d\d).+/, '$1'] 
			] 
			, blur : [ 
			// 날짜만 있을 때 월 붙이기 
			[/^(0?[1-9]|[1-2][0-9]|3[0-1])$/, y + '-' + ((m+1) < 10 ? '0'+(m+1) : (m+1) )+ '-' + (parseInt('$1') < 10 ? '0'+'$1' : '$1'), 1] 
			// 월-날짜 만 있을 때 연도 붙이기 
			, [/^(0?[1-9]|1[0-2])\-?(0[1-9]|[1-2][0-9]|3[01])$/, y+'-$1-$2'] 
			, [/^((?:0m?[1-9]|1[0-2])\-[1-9])$/, y+'-$1'] 
			// 연도 4 자리로 
			, [/^(\d\-\d\d?\-\d\d?)$/, y.substr(0, 3)+'$1', 1] 
			, [/^(\d\d\-\d\d?\-\d\d?)$/, y.substr(0, 2)+'$1', 1] 
			// 0 자리 붙이기 
			, [/(\d{4}-)(\d-\d\d?)$/, '$10$2', 1] 
			, [/(\d{4}-\d\d-)(\d)$/, '$10$2'] 
			] 
		} 

	event = event || window.event; 

	var input = event.target || event.srcElement 
		, value = input.value 
		, list = lists[event.type] 
		; 

	for(var i=0, c=list.length, match; i<c; i++){ 
		match = list[i]; 
		if(match[0].test(value)){ 
			input.value = value.replace(match[0], match[1]); 
			if(!match[2]) 
				break; 
		} 
	} 
} 

function getPreviewInfo(successFunc, errorFunc)
{
	var data = new HttpRequestData();
	data.type = "get";
	data.url = "./xml/preview.xml?time="+new Date();
	data.cache="false";
	data.cmdtype = 0;
	data.count = 0;

	sendHttpRequest(data, successFunc, errorFunc);

	delete(data);
}


/*
\brief : 
\param :
\return:
\note  :
*/
function setConfig(element, name, value, successFunc, errorFunc)
{
	var data = new HttpRequestData();
	data.type = "post";
	data.url = "./cgi-bin/vieworks.cgi?time="+new Date();
	data.cache="false";
	data.cmdtype = CMD_MODIFY_CONFIG;
	data.count = 2;
	data.userdata = element + "||" + name + "||" + value;

	sendHttpRequest(data, successFunc, errorFunc);

	delete(data);
}

function getConfig(successFunc, errorFunc)
{
	var data = new HttpRequestData();
	data.type = "get";
	data.url = "./xml/config.xml?time="+new Date();
	data.cache="false";
	data.cmdtype = 0;
	data.count = 0;

	sendHttpRequest(data, successFunc, errorFunc);

	delete(data);
}


function checkDateFormat(data)
{
	if(data.value=="")
		return;
	
	var d = getDate();
	var year = d.substr(0,4);
	var mon = d.substr(4,2);
	var day = d.substr(6,2);
	var temp =	data.value.split("-");
	if( temp.length != 3)
	{
		alert(_("INVLID_DATE_FORMAT"));
	}
	else if( parseInt(temp[0] + temp[1] + temp[2]) >  parseInt(d) )
	{
		alert(_("DATE_IS_AFTER_TODAY"));
	}
	else
		return;

	data.value = "";
	data.focus();
}

function getDetectorStatus(successFunc, errorFunc)
{
	var data = new HttpRequestData();
	data.type = "get";
	data.url = "./xml/status.xml?time="+new Date();
	data.cache="false";
	data.cmdtype = 0;
	data.count = 0;

	sendHttpRequest(data, successFunc, errorFunc);

	delete(data);
}


/*
\brief : 
\param :
\return:
\note  :
*/
function initPasswd(user, successFunc, errorFunc)
{
	var data = new HttpRequestData();
	data.type = "post";
	data.url = "./cgi-bin/vieworks.cgi?time="+new Date();
	data.cache="false";
	data.cmdtype = CMD_INIT_PASSWORD;
	data.count = 1;
	data.userdata = user;

	sendHttpRequest(data, successFunc, errorFunc);

	delete(data);
}


/*
\brief : 
\param :
\return:
\note  :
*/
function deleteStudy(id, name, accno,birth, studydate,studytime, successFunc, errorFunc)
{

	var data = new HttpRequestData();
	data.type = "post";
	data.url = "./cgi-bin/vieworks.cgi?time="+new Date();
	data.cache="false";
	data.cmdtype = CMD_DEL_ONE_STUDY;
	data.count = 4;
	data.userdata = id + "||" + name + "||" +  (accno == "" ? "none" : accno) + "||" +  (birth == "" ? "none" : birth) +"||" + studydate + "||" + studytime;

	sendHttpRequest(data, 
			function(data){
				successFunc(data);
			}
			, function(data){
				errorFunc(data);
			});

	delete(data);


}



/*
\brief : 
\param :
\return:
\note  :
*/
function invertImage(value, successFunc, errorFunc)
{

	var data = new HttpRequestData();
	data.type = "post";
	data.url = "./cgi-bin/vieworks.cgi?time="+new Date();
	data.cache="false";
	data.cmdtype = CMD_INVERT_IMAGE;
	data.count = 1;
	data.userdata = value;

	sendHttpRequest(data, 
			function(data){
				successFunc(data);
			}
			, function(data){
				errorFunc(data);
			});

	delete(data);


}

function use_for_human(value, successFunc, errorFunc)
{

	var data = new HttpRequestData();
	data.type = "post";
	data.url = "./cgi-bin/vieworks.cgi?time="+new Date();
	data.cache="false";
	data.cmdtype = CMD_USE_FOR_HUMAN;
	data.count = 1;
	data.userdata = value;

	sendHttpRequest(data, 
			function(data){
				successFunc(data);
			}
			, function(data){
				errorFunc(data);
			});

	delete(data);
}


