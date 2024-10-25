var loading=false;
var checkedList = "";
var select=false;
var timer="";
var StatusTimer="";
var user="";
var logoutFlag=false;
var for_human=true;

function displayPatientInfoToHTML()
{
	var date;
	var year, mon, day;
	var time;
	var hour,min,sec;
	var temp;
	var step;
	var key;
	var serial;
	var viewerserial;
	$("#menu1_img").html("");
	$("#menu1_img").css("top","0");

	temp = $(g_currentPatient).find("patient").find("info").attr("name");
	temp = temp.replace(/\^/g," ");
	if(temp !="")
		$("#menu1_txt_1").text(temp);
	else
		$("#menu1_txt_1").text(_("NONE"));
	
	temp = $(g_currentPatient).find("patient").find("info").attr("id");
	temp = temp.replace(/\+/g," ");
	if(temp !="")
		$("#menu1_txt_2").text(temp);
	else
		$("#menu1_txt_2").text(_("NONE"));

	temp = $(g_currentPatient).find("patient").find("info").attr("accno");
	temp = temp.replace(/\+/g," ");
	if(temp !="")
		$("#menu1_txt_3").text(temp);
	else
		$("#menu1_txt_3").text(_("NONE"));

	date = $(g_currentPatient).find("patient").find("info").attr("studydate");
	time = $(g_currentPatient).find("patient").find("info").attr("studytime");
	if(date!="" && time !="")
	{
		year = date.substr(0,4);
		mon = date.substr(4,2);
		day = date.substr(6,2);
		hour = time.substr(0,2);
		min = time.substr(2,2);
		sec = time.substr(4,2);
		$("#menu1_txt_4").text(year + "/" + mon + "/" + day + " " + hour + ":" + min + ":" + sec );
	}
	else
		$("#menu1_txt_4").text(_("NONE"));
	date = $(g_currentPatient).find("patient").find("info").attr("birth")
	if(date!="")
	{
		year = date.substr(0,4);
		mon = date.substr(4,2);
		day = date.substr(6,2);
		$("#menu1_txt_5").text(year + "/" + mon + "/" + day);
	}
	else
		$("#menu1_txt_5").text(_("NONE"));
	
	key = $(g_currentPatient).find("patient").find("step").attr("key");
	viewerserial = $(g_stepList).find("step").find("viewer").attr("serial");
	serial = $(g_currentPatient).find("patient").find("step").attr("viewerserial");
	if(key!="" && key!="0")
	{
		step = getStepInfoByKey(g_stepList,key);

		if( ((typeof(serial) != "undefined") && (serial != viewerserial)) || (step[0] == "") )
		{
			$("#menu1_txt_6").text(_("UNKNOWN"));
			$("#menu1_txt_7").text(_("UNKNOWN"));
		}
		else
		{
			$("#menu1_txt_6").text(step[0]);
			$("#menu1_txt_7").text(step[1]);
		}
	}
	else
	{
		$("#menu1_txt_6").text(_("NONE"));
		$("#menu1_txt_7").text(_("NONE"));
	}
}

function init()
{
	view("loading");
	getStepFromDetector();
	DetectorStatusTimer();
	joinDetector();
	ConfigSetting();
	getCurrentPatient( 
			function(data){
			
				g_currentPatient = data;
				displayPatientInfoToHTML();

			}, 
			function(data){

			});
	getPatientListFromDetector(function(){
		displayPatientList();
		hide("loading");
			});
	checkedList = "";
	var domParser = new DOMParser();
	checkedList = domParser.parseFromString("<?xml version=\"1.0\"?><step></step>", "text/xml");
	delete(domParser);
	$(".sub_nav").addClass("show");
}

function bclk(info, what)
{

	var i;
	switch(what)
	{
		case "1": // 환자선택 버튼
			clickNav(1, "on");
			clickNav(2, "off");

			break;
		case "2": // Step 선택 버튼
			displayStepInfo();

			/*
			$("dd:not(:first)").css("display","none"); 
			$("dl dt").click(function(){
					if($("+dd",this).css("display")=="none"){
					$(this).siblings("dd").slideUp("fast");
					$("+dd",this).slideDown("fast");
					}
					});

			$(".step_item").click(function(){
				//	var item = $(this);
					var key = $(this).find("span").text();
					setStepInfo(parseInt(key));
					displayPatientInfoToHTML();
					hidePopup();

					});


			$(".step_dt").addClass("step_bodypart");
			$(".step_item").addClass("step_bodypart");
			*/
			showPopup();
			break;
		case "3": // prieview 보기 버튼
		//	if($(g_currentPatient).find("patient").find("step").attr("key")=="")
		//	{
		//		alert("Select the step information.");
		//		break;
		//	}
			
			if($(g_currentPatient).find("patient").find("info").attr("id")=="")
			{
				var key = $(g_currentPatient).find("patient").find("step").attr("key")
				var serial = $(g_stepList).find("step").find("viewer").attr("serial");
				
				setEmergency();
				setStepInfo(key, serial);

				displayPatientInfoToHTML();	
			}

			$("#menu1_img").html("");
			$("#menu1_img").css("top","0");
			exposure("on");
			view("loading");
			sendInformation(function(data){
					timer = setTimeout("checkPreview()",1000);

					},
					function(data){
					hide("loading");
					exposure("off");
					});

			/*
			sendInformation(function(data){
					var filename = checkReturnValue(data,"ETC");
					//setTimeout(function(){pollImage(filename);},100);
					//pollImage(filename);
					var strHtml = "<img src='../preview/" + filename + "?time="+ new Date() + "' width='100%'></img>";
					$("#menu1_img").html(strHtml);
					hide("loading");

					$.scrollTo($("#menu1_img"), {
					duration: 750
					});

					//scroll();
					//window.scrollTo(0,500);
					markExpEnd(g_currentPatient);


					},
					function(data){

					});
*/

			break;
		case "4": //신규환자 추가 버튼
			var study;
			var birth=$('#menu2_2_birth').val();
			var temp;
			var etc = $('#menu2_2_etc').val() 
			var id = $('#menu2_2_id').val();
		    var name = $('#menu2_2_name').val();
			var accno = $('#menu2_2_accno').val();
			//var neutered = document.getElementById("neutered").value;
			var neutered = $('#neutered').val();
			var species = $('#menu2_2_3_species').val();
			var breed = $('#menu2_2_3_breed').val();
			var responsible = $('#menu2_2_3_responsible').val();
			
			if( for_human == true )
			{
				neutered = "";
				species = "";
				breed = "";
				responsible = "";
			}
			else
			{
				 etc = $('#menu2_2_3_etc').val()
				 id = $('#menu2_2_3_id').val();
				 name = $('#menu2_2_3_name').val();
				 accno = $('#menu2_2_3_accno').val();
				 birth=$('#menu2_2_3_birth').val();
			}
			// 환자 정보 입력

			if(checkBlank(id, _("INSERT_ID")) == false)
				return;
			else if(checkBlank(name, _("INSERT_NAME")) == false)
				return;
			/* Birth
			else if(checkBlank($("#menu2_2_birth").val(), "Insert the date of birth") == false)
				return;
				*/
		//	else if(checkBlank($("#menu2_2_accno").val(), "Insert the Acc No.") == false)
		//		return;
			else if(select == false)
			{
//				alert("Add step.");
//				return;
				select = false;
			}
			select = true;
			
			name = name.replace(/ /g,"^");
			id = id.replace(/ /g,"+");

			if( birth != "" )
			{
				temp = birth.split("-");
				if(temp[1]<10)
					temp[1] = '0' + parseInt(temp[1]);
				if(temp[2]<10)
					temp[2] = '0' + parseInt(temp[2]);

				birth = temp[0] + temp[1] + temp[2];

			}
			
			accno.replace(/ /g,"+");
			etc.replace(/ /g,"+");
			species.replace(/ /g,"+");
			breed.replace(/ /g,"+");
			responsible.replace(/ /g,"+");

			var date = getDate();
			var time = getTime();

			addPatient(
					name,
					birth,
					id,
					accno,
					etc,
					date,
					time,
					species,
					breed,
					responsible,
					neutered,
					function(data){
					// Step 정보 입력 
						$(checkedList).find("step").find("bodypart").each(function(){
							view("loading");
							$(this).find("projection").each(function(){
								addStepToPatient(id,name, accno,birth, date, time,$(this).attr("stepkey"));
								sleep(100); // 
								});
							});

						clickNav(1,"off");			
						setPatientInfo(id, name, accno,birth, date+"/"+time);
						setStepInfo("0", $(g_stepList).find("step").find("viewer").attr("serial"));
						displayPatientInfoToHTML();
						clickNav(2,"on");			

						hide("loading");
					},
					function(data){
					}
					);

		
				break;
		case "5": // 신규환자 추가 > Step 선택 버튼
			displayAddStep();
			showPopup();
			break;
		case "6": // 신규환자 추가 > Ste 선택 > OK or Cancel버튼
			if($(info).attr("id") == "menu2_2_chbox_ok")
			{
				var str="";
				str+= "<ul class='menu2_2_selected_step_ul'>";
				str+= "<li class='menu2_2_selected_step_li' ><span>" + _("BODYPART") + "</span><span>" + _("PROJECTION") + "</span></li>";
				var index=0;
				$(checkedList).find("bodypart").each(function(){
						$(this).find("projection").each(function(){
						var step = getStepInfoByKey(g_stepList,($(this).attr("stepkey")));
						if(index % 2 == 0)
						{
						str+= "<li class='menu2_2_selected_step_li menu2_2_selected_step_li_even' ><span>" + step[0] + "</span><span>" + step[1]  + "</span></li>";
						}
						else
						{
						str+= "<li class='menu2_2_selected_step_li' ><span>" + step[0] + "</span><span>" + step[1]  + "</span></li>";
						}
						index++;
						});

						});
					str+= "</ul>";
				if(index > 0)
				{
					$(".menu2_2_addstep").html(str);
					$(".menu2_2_toggle_btn").addClass("menu2_2_step_toggle");
					$(".menu2_2_toggle_btn").css("display","inline");;

					select = true;
					
				}
				else if(index == 0)
				{
					select = false;
					$(".menu2_2_addstep").html("");
					$(".menu2_2_toggle_btn").removeClass("menu2_2_step_toggle");
					$(".menu2_2_toggle_btn").css("display","none");;
				}

			}
			else
				removeCheckedAll();

			hidePopup();
			break;
		case "7":
			hidePopup();
			break;
		case "8":
				if($(info).hasClass("menu2_2_step_toggle"))
					$(info).removeClass("menu2_2_step_toggle");
				else
					$(info).addClass("menu2_2_step_toggle");
				$(".menu2_2_selected_step_ul").slideToggle("fast");
			break;
		case "9":
			setEmergency();
			displayPatientInfoToHTML();	
			break;
		case "10":
			if($(info).attr("id") == "menu3_sub_ok")
			{
				var passwd = $("#menu3_sub_password").val();
				var newpasswd = $("#menu3_sub_newpassword").val();
				var newpasswdagain = $("#menu3_sub_newpasswordagain").val();

				if(checkBlank($("#menu3_sub_password").val(), _("INSERT_PASSWORD")) == false)
					return;
				else if(checkBlank($("#menu3_sub_newpassword").val(), _("INSERT_NEW_PASSWORD")) == false)
					return;
				else if(checkBlank($("#menu3_sub_newpasswordagain").val(), _("INSERT_NEW_PASSWORD_AGAIN")) == false)
					return;

				if(newpasswd != newpasswdagain)
				{
					alert(_("DOES_NOT_MATCH_PASSWORD"));
					$("#menu3_sub_password").val("");
					$("#menu3_sub_newpassword").val("");
					$("#menu3_sub_newpasswordagain").val("");
					$("#menu3_sub_password").focus();
					return;
				}
				changePasswd(user, 
						hex_sha1(passwd.toLowerCase()).toUpperCase(), 
					hex_sha1(newpasswd.toLowerCase()).toUpperCase(),
						function(data){
							var ret = checkReturnValue(data,"VALUE");
							if(ret == 0)
							{
								alert(_("PASSWORD_CHANGE_SUCCESS"));
								menu3_sub(false,"");
								return;
							}
							else if(ret == 1)
							{
								alert(_("CANNOT_READ_PASSWORD"));
							}
							else if(ret == 2)
							{
								alert(_("CHECK_PASSWORD"));
														}
							else if(ret == 3)
							{
								alert(_("CANNOT_CHANGE_PASSWORD"));
							}
							else if(ret == 4)
							{
								alert(_("CANNOT_WRITE_NEW_PASSWORD"));
							}
							else 
							{
								alert(_("UNKNOWN_ERROR"));
							}

							$("#menu3_sub_password").val("");
							$("#menu3_sub_newpassword").val("");
							$("#menu3_sub_newpasswordagain").val("");
							$("#menu3_sub_password").focus();

						},
						function(data){
							alert("Unknown error occurred.");
							$("#menu3_sub_password").val("");
							$("#menu3_sub_newpassword").val("");
							$("#menu3_sub_newpasswordagain").val("");
							$("#menu3_sub_password").focus();

						});
			}
			else if($(info).attr("id") == "menu3_sub_cancel")
			{
				menu3_sub(false,"");
			}
			break;

	}
}

function addChecked(key)
{
//	var newkey = document.createElement("key");
//	$(newkey).attr("value",key);
//	$(checkedList).find("step").append($(newkey));

	var step = getStepInfoByKey(g_stepList,key);
	addPatientStepList(checkedList, step[0], step[1], key);
}

function removeChecked(key)
{
//	$(checkedList).find("step").find("key").each(function(){
//			if($(this).attr("value") == key)
//				$(this).remove();
//			});
	$(checkedList).find("step").find("bodypart").each(function(){
			var cnt=0;
			$(this).find("projection").each(function(){
					cnt++;
					if($(this).attr("stepkey") == key)
					{
						$(this).remove();
						cnt--;
					}

				});
				if(cnt == 0)	
					$(this).remove();
			});
}

function removeCheckedAll()
{
//	$(checkedList).find("step").find("key").each(function(){
//				$(this).remove();
//
//			});
	$(checkedList).find("step").find("bodypart").each(function(){
			$(this).find("projection").each(function(){
					$(this).remove();
				});
				$(this).remove();
				});
	$(".menu2_2_addstep").html("");
	$(".menu2_2_toggle_btn").removeClass("menu2_2_step_toggle");
	$(".menu2_2_toggle_btn").css("display","none");;

}

function showPopup()
{
	$(".popup_container").addClass("show");
}

function hidePopup()
{
	$(".popup_container").removeClass("show");

}


function displayAddStep()
{

	var str = "<ul class='step_item_nav'><li class='step_item_nav_l step_item_nav_two click' id='step_item_nav_l_1'>" + _("FAVORITE") +"</li>";
	str+="<li class='step_item_nav_l step_item_nav_two unclick' id='step_item_nav_l_2'>" + _("ALL") + "</li>";
	$(".popup_nav").html(str);
	$(".popup_header").html(_("ADD_BODYPART"));
	str="<button type='button' class='menu2_2_chbox_btn btn_popup' value='OK' id='menu2_2_chbox_ok' onclick='bclk(this,\"6\")'>" + _("OK") + "</button>";
	$(".popup_footer").html(str);

	$(".step_item_nav_l").click(function(){
			$(".step_item_nav_l").each(function(){
				$(this).removeClass("click");
				$(this).addClass("unclick");
			});
			$(this).removeClass("unclick");
			$(this).addClass("click");
			var id = $(this).attr("id");
			id = id.substr(16,1);	
			if(id == "1")
				displayAddStepImpl(g_favoriteStepList);
			else if(id=="2")
				displayAddStepImpl(g_stepList);

			});


	displayAddStepImpl(g_favoriteStepList);

}

function displayAddStepImpl(stepList)
{
	var str="";
	var validStep = false;
	str="<dl class='step_dl'>\n";
	$(stepList).find("step").find("bodypart").each(function(){
			str+="<dt class='step_dt'>" + $(this).attr("meaning") + "</dt>\n";	
			str+="<dd>";
			var index=0;
			$(this).find("projection").each(function(){
				validStep = true;
				
				if(index % 2 == 0)
				{
					str+="<p class='step_item step_item_even'><span style='width:70%'>" + $(this).attr("meaning") + "</span>" ;	
					str+="<span><input class='menu2_2_chbox' id='menu2_2_2_chbox_" + $(this).attr("stepkey") + "' type='checkbox' name='selected' value='" + $(this).attr("stepkey") +"'></input></span></p>\n";	
				}
				else
				{
					str+="<p class='step_item'><span style='width:70%'>" + $(this).attr("meaning")  + "</span>";	
					str+="<span><input class='menu2_2_chbox' id='menu2_2_2_chbox_" + $(this).attr("stepkey") + "' type='checkbox' name='selected' value='" + $(this).attr("stepkey") +"'></input></span></p>\n";	
				}
				index++;
				});
			str+="</dd>";
			});

	str+="</dl>\n";

	if(validStep == false)
		str = "<br><br><center>" + _("BODYPART_NOT_EXIST") + "</center>";

	$(".popup_body").html(str);

	$("input[name=selected]").each(function(){
				var doc = document.getElementById($(this).attr("id"));
				doc.checked = isChecked($(this).attr("value"));

			});

	$("dd:not(:first)").css("display","none"); 
	$("dl dt").click(function(){
			if($("+dd",this).css("display")=="none"){
			$(this).siblings("dd").slideUp("fast");
			$("+dd",this).slideDown("fast");
			}
			});
	$(".menu2_2_chbox").click(function(){
			var doc = document.getElementById($(this).attr("id"));
			if(doc.checked == true)
			doc.checked = false;
			else
			doc.checked = true;

			});

	$(".step_item").click(function(){
			var doc = document.getElementById($(this).find("input").attr("id"));	
			if(doc.checked == true)
			{
			doc.checked = false;
			removeChecked($(this).find("input").attr("value"));
			}
			else
			{
			doc.checked = true;
			addChecked($(this).find("input").attr("value"));
			}
			});
	$(".step_dt").addClass("step_bodypart");
	$(".step_item").addClass("step_bodypart");

}



function displayStepInfo()
{
	var str = "<ul class='step_item_nav'><li class='step_item_nav_l step_item_nav_three click' id='step_item_nav_l_1'><a>" + _("SELECTED") + "</a></li>";
	str+="<li class='step_item_nav_l step_item_nav_three unclick' id='step_item_nav_l_2'><a>" + _("FAVORITE") + "</a></li>";
	str+="<li class='step_item_nav_l step_item_nav_three unclick' id='step_item_nav_l_3'><a>" + _("ALL") + "</a></li></ul>";
	$(".popup_nav").html(str);
	$(".popup_header").html(_("BODYPART_SELECTION"));
	str="<button type='button' class='menu1_step_btn btn_popup' value='Cancel' id='menu2_2_chbox_ok' onclick='bclk(this,\"7\")'>" + _("CANCEL") + "</button>";
	$(".popup_footer").html(str);

	$(".step_item_nav_l").click(function(){
			$(".step_item_nav_l").each(function(){
				$(this).removeClass("click");
				$(this).addClass("unclick");
			});
			$(this).removeClass("unclick");
			$(this).addClass("click");

			var stepList;
			var id = $(g_currentPatient).find("patient").find("info").attr("id");
			var name = $(g_currentPatient).find("patient").find("info").attr("name");
			var accno = $(g_currentPatient).find("patient").find("info").attr("accno");
			var birth = $(g_currentPatient).find("patient").find("info").attr("birth");
			var study = $(g_currentPatient).find("patient").find("info").attr("studydate") + "/" +$(g_currentPatient).find("patient").find("info").attr("studytime") ;
		
			var idx = $(this).attr("id");
			idx = idx.substr(16,1);	
			if(idx == "1")
			{
				stepList = getPatientStepInfo(id,name,accno,birth,study);
//				if($(stepList).find("step").find("bodypart").length < 1)
//					stepList = "";

				displayStepInfoImpl(stepList, id, name, accno,birth, study);
			}
			else if(idx =="2")
				displayStepInfoImpl(g_favoriteStepList, id, name, accno,birth, study);
			else if(idx =="3")
				displayStepInfoImpl(g_extendedStepList, id, name, accno,birth, study);


			});


	var stepList;
	var id = $(g_currentPatient).find("patient").find("info").attr("id");
	var name = $(g_currentPatient).find("patient").find("info").attr("name");
	var accno = $(g_currentPatient).find("patient").find("info").attr("accno");
	var birth = $(g_currentPatient).find("patient").find("info").attr("birth");
	var study = $(g_currentPatient).find("patient").find("info").attr("studydate") + "/" +$(g_currentPatient).find("patient").find("info").attr("studytime") ;
	stepList = getPatientStepInfo(id,name,accno,birth,study);
//	if($(stepList).find("step").find("bodypart").length < 1)
//	{
//		stepList = "";
//	}

	displayStepInfoImpl(stepList, id, name, accno,birth, study);

}

function displayStepInfoImpl(stepList, id, name, accno,birth, study)
{
	/*
	var stepList;
	var str="";
	var id = $(g_currentPatient).find("patient").find("info").attr("id");
	var name = $(g_currentPatient).find("patient").find("info").attr("name");
	var accno = $(g_currentPatient).find("patient").find("info").attr("accno");
	var study = $(g_currentPatient).find("patient").find("info").attr("studydate") + "/" +$(g_currentPatient).find("patient").find("info").attr("studytime") ;
	stepList = getPatientStepInfo(id,name,accno,study);
	if($(stepList).find("step").find("bodypart").length < 1)
	{
		stepList = g_stepList;
		// 환자 정보가 없기 때문에 default step정보
	}
	*/
	
	var str="";
	var stepkey="";
	var step="";
	var meaning="";
	var validStep=false;
	var expEnd;
	var	dlElement;
	var	dtElement;
	var	ddElement;
	var	pElement;
	var	spanElement;
	var	brElement;
	var	centerElement;
	var index=0;

	var body = document.getElementById("popup_body");
	while(body.childNodes.length > 0) 
		body.removeChild(body.childNodes[0]);

	dlElement = document.createElement("dl");
	var viewerserial = $(g_stepList).find("step").find("viewer").attr("serial");
	$(stepList).find("step").find("bodypart").each(function(){
			dtElement = document.createElement("dt");
			dtElement.className = "step_dt";
			dtElement.innerText = $(this).attr("meaning") =="" ? _("UNKNOWN") :$(this).attr("meaning");
			index = 0;

			ddElement = document.createElement("dd");
			$(this).find("projection").each(function(){
				var expEnd = "true";
				if(id!="")
					expEnd = checkExpEndBIDnKey(id,name,accno,birth, study,$(this).attr("stepkey"));

				stepkey = $(this).attr("stepkey");
				meaning = $(this).attr("meaning");	
				serial = $(this).attr("viewerserial");	
				step = getStepInfoByKey(g_stepList, stepkey);
				if( ((typeof(serial) == "undefined") || (serial == viewerserial)) && (step[0] != "") )
				{
					validStep = true;
					pElement = document.createElement("p");
					pElement.className = "step_item";
					if(index % 2 == 0)
						pElement.className += " step_item_even";
					if(expEnd == "false")
						pElement.className += " step_item_complete";
					pElement.innerText = meaning;

					spanElement = document.createElement("span");
					spanElement.id = "step_item_stepkey";
					spanElement.style.visibility = "hidden";
					spanElement.style.width = "0";
					spanElement.innerText = stepkey;
					pElement.appendChild(spanElement);

					spanElement = document.createElement("span");
					spanElement.id = "step_item_viewerserial";
					spanElement.style.visibility = "hidden";
					spanElement.style.width = "0";
					spanElement.innerText = serial;
					pElement.appendChild(spanElement);

					spanElement = document.createElement("span");
					spanElement.style.float = "right";
					spanElement.innerHTML = "&nbsp";
					pElement.appendChild(spanElement);

					ddElement.appendChild(pElement);

					index++;
				}
				});

			if(ddElement.hasChildNodes())
			{
				dlElement.appendChild(dtElement);
				dlElement.appendChild(ddElement);
			}
		});
	
	if(validStep == false)
	{
		brElement = document.createElement("br");
		centerElement = document.createElement("center");
		centerElement.innerText = _("BODYPART_NOT_EXIST");
		
		document.getElementById("popup_body").appendChild(brElement);
		document.getElementById("popup_body").appendChild(brElement);
		document.getElementById("popup_body").appendChild(centerElement);
	}
	else
		document.getElementById("popup_body").appendChild(dlElement);



	/*
	str="<dl class='step_dl'>\n"; 
	var viewerserial = $(g_stepList).find("step").find("viewer").attr("serial");
	$(stepList).find("step").find("bodypart").each(function(){
			if($(this).attr("meaning") =="")
				str+="<dt class='step_dt'>" + _("UNKNOWN") + "</dt>\n";	
			else 
				str+="<dt class='step_dt'>" + $(this).attr("meaning") + "</dt>\n";	
			str+="<dd>";
			var index=0;
			$(this).find("projection").each(function(){
				validStep = true;
				var expEnd = "true";
				if(id!="")
					expEnd = checkExpEndBIDnKey(id,name,accno,birth, study,$(this).attr("stepkey"));


				stepkey = $(this).attr("stepkey");
				meaning = $(this).attr("meaning");	
				serial = $(this).attr("viewerserial");	
				step = getStepInfoByKey(g_stepList, stepkey);
				if( ((typeof(serial) != "undefined") && (serial != viewerserial)) || (step[0] == "") )
					meaning = _("UNKNOWN");

				if(index % 2 == 0)
				{
					if(expEnd == "false")
					{
					str+="<p class='step_item step_item_even step_item_complete'>" + meaning ;	
					str+="<span id='step_item_stepkey' style='visibility:hidden; width:0;'>" + stepkey + "</span><span id='step_item_viwerserial' style='visibility:hidden; width:0;'>" + serial + "</span><span style='float:right;'>&nbsp;</span></p>\n";	
					}
					else
					{
					str+="<p class='step_item step_item_even'>" + meaning ;	
					str+="<span id='step_item_stepkey' style='visibility:hidden; width:0;'>" + stepkey + "</span><span id='step_item_viwerserial' style='visibility:hidden; width:0;'>" + serial + "</span><span style='float:right;'>&nbsp;</span></p>\n";	
					}
				}
				else
				{
					if(expEnd == "false")
					{
					str+="<p class='step_item step_item_complete'>" + meaning;	
					str+="<span id='step_item_stepkey' style='visibility:hidden; width:0;'>" + stepkey + "</span><span id='step_item_viwerserial' style='visibility:hidden; width:0;'>" + serial + "</span><span style='float:right;'>&nbsp;</span></p>\n";	
					}
					else
					{
					str+="<p class='step_item'>" + meaning ;	
					str+="<span id='step_item_stepkey' style='visibility:hidden; width:0;'>" + stepkey + "</span><span id='step_item_viwerserial' style='visibility:hidden; width:0;'>" + serial + "</span><span style='float:right;'>&nbsp;</span></p>\n";	
					}
				}
				index++;
				});
			str+="</dd>";
			});

	str+="</dl>\n";

	if(validStep == false)
		str = "<br><br><center>" + _("BODYPART_NOT_EXIST") + "</center>";

	$(".popup_body").html(str);
	*/

	$("dd:not(:first)").css("display","none"); 
	$("dl dt").click(function(){
			if($("+dd",this).css("display")=="none"){
			$(this).siblings("dd").slideUp("fast");
			$("+dd",this).slideDown("fast");
			}
			});

	$(".step_item").click(function(){
			//	var item = $(this);
			var key;
			var serial;
			$(this).find("span").each(function(){

				if( $(this).attr("id") == "step_item_stepkey")
				{
					key = $(this).text();
				}
				else if( $(this).attr("id") == "step_item_viewerserial")
				{
					serial = $(this).text();
				}


				});
			setStepInfo(parseInt(key), serial);
			displayPatientInfoToHTML();
			hidePopup();

			});


	$(".step_dt").addClass("step_bodypart");
	$(".step_item").addClass("step_bodypart");

}

function displayPatientList()
{
	var list = g_patientList;
	var str="";
	var patient="";
	var birth, study, date, time, year, mon, day, hour, min, sec;
	var accno;
	var reserve;
	var name, id;

	$(list).find("list").find("patient").each(function(){
			patient = $(this);
			str+= "<div class='menu2_1_list_frame'>";
			str+= "<div class='menu2_1_list_contain'>";
			str+= "<div class='menu2_1_list_body'>";
			id = $(this).attr("id").replace(/\+/g," ");
			str+= "<ul class='menu2_1_list_ul' id='" + id + "'>";
			str+= "<li id='menu2_1_list_li_id' >" + _("PATIENT_ID") + " : " + id + "<span class='hidden'>" +  $(this).attr("id") + "</span></li>";
			name = $(this).attr("name").replace(/\^/g," ");
			str+= "<li id='menu2_1_list_li_name' >" + _("NAME") + " : " + name + "<span class='hidden'>" +  $(this).attr("name") + "</span></li>";

			date = 	$(this).attr("birth") ;
			if(date!="")
			{
				year = date.substr(0,4);
				mon = date.substr(4,2);
				day = date.substr(6,2);
				birth = year + "/" + mon + "/" + day ;
			}
			else
				birth = _("NONE");

			str+= "<li id='menu2_1_list_li_birth' >" + _("BIRTH") + " : " + birth + "<span class='hidden'>" +  $(this).attr("birth") + "</span></li>";

			accno =  $(this).attr("accno").replace(/\+/g," ");
			if(accno=="")
				accno = _("NONE");
			str+= "<li id='menu2_1_list_li_accno'>" + _("ACCNO") + " : " + accno + "<span class='hidden'>" +  $(this).attr("accno") + "</span></li>";

			date = 	$(this).attr("studydate") ;
			time = 	$(this).attr("studytime") ;
			if(date!="" && time!="")
			{
				year = date.substr(0,4);
				mon = date.substr(4,2);
				day = date.substr(6,2);
				hour = time.substr(0,2);
				min = time.substr(2,2);
				sec = time.substr(4,2);
				study = year + "/" + mon + "/" + day + " " + hour + ":" + min + ":" + sec;
			}
			else
				study = "none";
			str+= "<li id='menu2_1_list_li_study' >" + _("STUDY_TIME") + " : " + study +  "<span class='hidden'>" +  $(this).attr("studydate")  + "/" + $(this).attr("studytime") + "</span></li>";
			
			reserve =  $(this).attr("reserve").replace(/\+/g," ");;
			if(reserve=="")
				reserve = _("NONE");
			str+= "<li style='display:none'>" + _("ETC") + " : " + reserve + "</li>";

			species =  $(this).attr("species_description").replace(/\+/g," ");
			if(species!="" && typeof(species)!="undefined")
				str+= "<li style='display:none'>" + _("SPECIES_DESCRIPTION") + " : " + species + "</li>";
			
			breed =  $(this).attr("breed_description").replace(/\+/g," ");
			if(breed!="" && typeof(breed)!="undefined")
				str+= "<li style='display:none'>" + _("BREED_DESCRIPTION") + " : " + breed + "</li>";

			person =  $(this).attr("responsible_person").replace(/\+/g," ");
			if(person!="" && typeof(person)!="undefined")
				str+= "<li style='display:none'>" + _("RESPONSIBLE_PERSON") + " : " + person + "</li>";

			neutered =  $(this).attr("neutered");
			if( for_human == false )
			{
				if( neutered!="" && typeof(neutered)!="undefined" )
				{
					str+= "<li id='menu2_1_list_li_neutered' style='display:none'>" + _("NEUTERED") + neutered + "</li>";
				}
			}

			str+= "</ul>";
			str+= "</div>";
			str+= "<div class='menu2_1_list_toggle'>";
			str+= "<ul class='menu2_1_list_toggle_ul'>";
			if($(this).attr("enable") == "false")
				str+= "<li class='menu2_1_list_toggle_li menu2_1_list_toggle_complete'>&nbsp</li>";
			else
				str+= "<li class='menu2_1_list_toggle_li'>&nbsp</li>";
			str+= "<li class='menu2_1_list_toggle_li menu2_1_list_toggle_btn'>&nbsp;</li>";
			str+= "</ul>";
			str+= "</div>";
			str+= "</div>";
			str+= "<div class='menu2_1_list_step'>";
			str+= "<ul class='menu2_1_step_ul'>";
			var index=0;
			var stepkey="";
			var bodypart="";
			var projection="";
			var stepList = getPatientStepInfo($(this).attr("id"),$(this).attr("name"),$(this).attr("accno"), $(this).attr("birth"),($(this).attr("studydate")  + "/" + $(this).attr("studytime")));
			if($(stepList).find("step").find("bodypart").length > 0)
			{
			str+= "<li style='display:none'>" + _("STEP_INFO") + "</li>";
			str+= "<li class='menu2_1_step_li menu2_1_step_label' style='display:none'><span>" + _("BODYPART") + "</span><span>" + _("PROJECTION") + "</span><span>" + _("COMPLETE") + "</span></li>";
			var serial = $(g_stepList).find("step").find("viewer").attr("serial");
			$(stepList).find("step").find("bodypart").each(function(){
			$(this).find("projection").each(function(){
				var step = getStepInfoByKey(g_stepList,$(this).attr("stepkey")); 
				var viewerSerial = $(this).attr("viewerserial");
				stepkey = $(this).attr("stepkey");
				if(stepkey == "0")
				{
					bodypart = _("NONE");
					projection = _("NONE");
				}
				else if( ((typeof(viewerSerial) == "undefined") || (serial == viewerSerial)) && (step[0] !="") )
				{
					bodypart = step[0];
					projection = step[1];
				}
				else if( (serial != viewerSerial) || ( (step[0] == "") ) )
				{
					bodypart = _("UNKNOWN");
					projection = _("UNKNOWN");
				}

				if(typeof(viewerSerial) == "undefined") 
					viewerserial = serial;
				
			
				var expEnd = "true";
				expEnd = checkExpEndBIDnKey($(patient).attr("id"),$(patient).attr("name"),$(patient).attr("accno"), $(patient).attr("birth"), ($(patient).attr("studydate")  + "/" + $(patient).attr("studytime")),$(this).attr("stepkey"));
				if(index % 2 == 0)
				{
					if(expEnd == "false")
						str+= "<li class='menu2_1_step_li menu2_1_step_item menu2_1_step_item_even menu2_1_step_item_complete' style='display:none' id='"+ $(patient).attr("id") +"' key='" + $(this).attr("stepkey") + "' viewerserial='" + $(this).attr("viewerserial") + "'><span>" + bodypart + "</span><span>" + projection  + "</span><span>&nbsp;</span></li>";
					else
						str+= "<li class='menu2_1_step_li menu2_1_step_item menu2_1_step_item_even' style='display:none' id='"+ $(patient).attr("id") +"' key='" + $(this).attr("stepkey") + "' viewerserial='" + $(this).attr("viewerserial") + "'><span>" + bodypart + "</span><span>" + projection  + "</span><span>&nbsp;</span></li>";
					}
				else
				{
					if(expEnd == "false")
						str+= "<li class='menu2_1_step_li menu2_1_step_item menu2_1_step_item_complete' style='display:none' id='"+ $(patient).attr("id") +"' key='" + $(this).attr("stepkey") + "' viewerserial='" + $(this).attr("viewerserial") + "'><span>" + bodypart + "</span><span>" + projection  + "</span><span>&nbsp;</span></li>";
					else
						str+= "<li class='menu2_1_step_li menu2_1_step_item' style='display:none' id='"+ $(patient).attr("id") +"' key='" + $(this).attr("stepkey") + "' viewerserial='" + $(this).attr("viewerserial") + "'><span>" + bodypart + "</span><span>" + projection  + "</span><span>&nbsp;</span></li>";
					}
				index++;
				});
			});
			}
			str+= "</ul>";
			str+= "</div>";
			str+= "</div>";
			});
	
	$(".menu2_1").html(str);

	var list_idx = 0;
	$(".menu2_1_list_frame").each(function(){
			if($(this).find("div").find("div").find("ul").find("li").hasClass("menu2_1_list_toggle_complete"))
			{
				$(this).find("div").each(function(){
					if($(this).hasClass("menu2_1_list_contain"))
						$(this).addClass("menu2_1_list_frame_even");

					});
			}
			});
	
	$(".menu2_1_list_toggle_btn").click(function(){
			if($(this).hasClass("toggle_open") == false)
				$(this).addClass("toggle_open");
			else
				$(this).removeClass("toggle_open");
				
				
			var step = $(this).parent().parent().parent().siblings("div").find("ul").find("li");
			if($(step).length < 2)
				$(step).slideToggle("fast");
			else
				$(step).siblings("li").slideToggle("fast");
				
			$(this).parent().parent().siblings("div").find("ul").each(function(){
				var index=0;
				$(this).find("li").each(function(){
					if(index > 4)
					$(this).slideToggle("fast");

					index+=1;
					});
				});
			});

	$(".menu2_1_step_item").click(function(){
			var id;
			var name;
			var accno;
			var study;
			var neutered;
			$(this).parent().parent().siblings("div").find("div").each(function(){
					if($(this).hasClass("menu2_1_list_body") == true)
					{
						$(this).find("ul").find("li").each(function(){
							var item = $(this);
							var span = $(this).find("span");
							if($(item).attr("id") == "menu2_1_list_li_name")
								name = $(span).text();
							else if($(item).attr("id") == "menu2_1_list_li_id")
								id = $(span).text();
							else if($(item).attr("id") == "menu2_1_list_li_accno")
								accno = $(span).text();
							else if($(item).attr("id") == "menu2_1_list_li_study")
								study = $(span).text();
							else if($(item).attr("id") == "menu2_1_list_li_birth")
								birth = $(span).text();
							});
					}
				});
			setPatientInfo(id, name, accno,birth, study);
			setStepInfo(parseInt($(this).attr("key")), $(this).attr("viewerserial"));

			clickNav(1,"off");
			clickNav(2,"on");
			});


//* 환자 목록에서 리스트 클릭 	
	$(".menu2_1_list_ul").click(function(){
			var id, name, accno, study, birth, neutered;
			var serial = $(g_stepList).find("step").find("viewer").attr("serial");
			$(this).find("li").each(function(){
				if($(this).attr("id") == "menu2_1_list_li_id")
					id = $(this).find("span").text();
				else if($(this).attr("id") == "menu2_1_list_li_name")
					name = $(this).find("span").text();
				else if($(this).attr("id") == "menu2_1_list_li_accno")
					accno = $(this).find("span").text();
				else if($(this).attr("id") == "menu2_1_list_li_study")
					study = $(this).find("span").text();
				else if($(this).attr("id") == "menu2_1_list_li_birth")
					birth = $(this).find("span").text();

				});

			setPatientInfo(id, name, accno,birth, study);
			setStepInfo("0",serial);
			clickNav(1,"off");
			clickNav(2,"on");
			});

	$(".menu2_1_list_ul").longclick(function(){
			var ret = confirm(_("DELETE_SELECTED_STUDY_CONFIRM"));
			if(ret == true)
			{
				view("loading");
				var id, name, accno, study, tmp, birth;
				$(this).find("li").each(function(){
					if($(this).attr("id") == "menu2_1_list_li_id")
						id = $(this).find("span").text();
					else if($(this).attr("id") == "menu2_1_list_li_name")
						name = $(this).find("span").text();
					else if($(this).attr("id") == "menu2_1_list_li_accno")
						accno = $(this).find("span").text();
					else if($(this).attr("id") == "menu2_1_list_li_study")
						tmp = $(this).find("span").text();
					else if($(this).attr("id") == "menu2_1_list_li_birth")
						birth = $(this).find("span").text();

			

				});
				study = tmp.split('/');            
				deleteStudy(id, name, accno,birth, study[0], study[1],
					function(data){
					
						getPatientListFromDetector(function(data){
							displayPatientList();
							hide("loading");
						});
					},
					function(data){
						hide("loading");

					});
			}
		});

}

function clickNav(nav, onoff)
{
	var menu = nav;
	if(menu != 3)
		menu = (nav == 2) ? 1 : 2;
	if(onoff == "on")
	{
		if(nav == 1)
		{
			view("loading");
			getPatientListFromDetector(function(data){
					displayPatientList();
					hide("loading");
					});
			$(".sub_nav").addClass("show");
		}
		else if(nav == 2)
		{
			displayPatientInfoToHTML();
		}

		$(".nav_" + nav).addClass("click");
		$("#menu"+ menu).addClass("show");
	}
	else
	{
		if(nav == 1)
		{
			$(".menu2_2_addstep").html("");
			$(".menu2_2_toggle_btn").removeClass("menu2_2_step_toggle");
			$(".menu2_2_toggle_btn").css("display","none");;
			removeCheckedAll();

			$(".sub_nav_2").removeClass("click");
			$(".sub_nav_3").removeClass("click");
			$(".menu2_2").removeClass("show");
			$(".menu2_2_3").removeClass("show");

			$(".sub_nav_1").addClass("click");
			$(".menu2_1").addClass("show");

			$(".sub_nav").removeClass("show");
		}
		$(".nav_"+nav).removeClass("click");
		$("#menu"+menu).removeClass("show");
	}
}

function menu3_sub(onoff, str)
{
	if(onoff == true)
	{
		$("#menu3_main").removeClass("show");
		$("#menu3_main").addClass("hide");

		$("#menu3_sub").html(str);

		$("#menu3_sub").removeClass("hide");
		$("#menu3_sub").addClass("show");

	}
	else if(onoff == false)
	{
		$("#menu3_sub").removeClass("show");
		$("#menu3_sub").addClass("hide");

		$("#menu3_sub").html("");

		$("#menu3_main").removeClass("hide");
		$("#menu3_main").addClass("show");
	}
}

function isChecked(key)
{
	var checked = false;
	$(checkedList).find("step").find("bodypart").each(function(){
			$(this).find("projection").each(function(){
				if($(this).attr("stepkey") == key)
					checked = true;
				});
			});
	return checked;
}

function checkBlank(value, str)
{
	if(value=="")
	{
		alert(str);
		return false;
	}

	return true;

}

function setEmergency()
{
	var date = getDate();
	var time = getTime();
	var id = "EM-"+date+"-" + time;
	var name = "Urgent^Patient";

	setEmergencyPatient(id, name, date,time,date);
}

function checkPreview()
{
	getPreviewInfo(function(data){
					var strHtml="";
					var id, name, accno,birth, studydate, studytime;
					var info = $(data).find("preview").find("info");
					id = $(info).attr("id").replace(/\+/g," ");
					name = $(info).attr("name").replace(/\^/g," ");
					accno = $(info).attr("accno").replace(/\+/g," ");
					birth = $(info).attr("birth");
					studydate = $(info).attr("studydate");
					studytime = $(info).attr("studytime");
					
					var key = $(data).find("preview").find("step").attr("key");
					var time = $(data).find("preview").find("image").attr("time");
					if(isWatingInfo(id, name, accno,birth, studydate, studytime, key) == true)
					{
						strHtml = "<img src='../preview/preview_" + time + ".bmp?time="+ new Date() + "' width='100%'></img>";

						$("#menu1_img").html(strHtml);
	
						$.scrollTo($("#menu1_img"), {
						duration: 750
						});

						getPatientListFromDetector(function(data){});
	
						hide("loading");
						exposure("off");

					}
					else
					{
						strHtml="<center><p><h3>" + _("IMAGE_DOES_NOT_MATCH_PATIENT_INFO") + "</h3><h3>" + _("OTHER_DEVICES") + "</h3></p></center>"
						$("#menu1_img").html(strHtml);
						hide("loading");
						exposure("off");
					}

			},
			function(data){
				if(timer != -1)
					timer = setTimeout("checkPreview()",1000);
			}
			);

}

function isWatingInfo(id, name, accno,birth, studydate, studytime, key)
{
	var currentId, currentName, currentAccno, currentStudydate, currentStudytime, currentKey, currentBirth;
	currentName = $(g_currentPatient).find("patient").find("info").attr("name").replace(/\^/g," ");;
	currentId = $(g_currentPatient).find("patient").find("info").attr("id").replace(/\+/g," ");;
	currentAccno = $(g_currentPatient).find("patient").find("info").attr("accno").replace(/\+/g," ");;
	currentBirth = $(g_currentPatient).find("patient").find("info").attr("birth");
	currentStudydate= $(g_currentPatient).find("patient").find("info").attr("studydate");
	currentStudytime= $(g_currentPatient).find("patient").find("info").attr("studytime");
	currentKey = $(g_currentPatient).find("patient").find("step").attr("key");

	if(currentId == id &&
			currentName == name &&
			currentAccno == accno &&
			currentBirth == birth &&
			currentStudydate == studydate &&
			currentStudytime == studytime &&
			currentKey == key)
	{
		return true;
	}

	return false;
}

function DetectorStatusTimer()
{
	getDetectorStatus(function(data){
			var filename="";
			var info = $(data).find("status");
			var remain = $(info).find("bat").attr("remain");
			var cnt = $(info).find("image").attr("cnt");
			var total = $(info).find("image").attr("total");

			
			if(remain > 10)
				$("#status_l_bat").html("B:" + remain +"%");
			else if(remain > 1)
				$("#status_l_bat").html("B:" + _("LOW"));
			$("#status_l_cnt").html("I:" + cnt + "/" + total);

			}, function(data){

			$("#status_l_bat").html("B:none");
			$("#status_l_cnt").html("I:none");

			});

	checkSess();
	StatusTimer = setTimeout("DetectorStatusTimer()",5000);
}

function logoutSess()
{
	logout(function(data){
			window.location = "login.html";
		//	alert("Good bye!");

			}, function(data){

			});


}

function unloadPage()
{
	logout(function(data){
				alert(_("GOOD_BYE"));

			}, function(data){

				});

}

function check_usage(data)
{
	if($(data).find("config").find("usage").attr("human") == "true")
	{
		for_human = true;
	}
	else
	{
		for_human = false;
	}
}


// onload
$(document).ready(function(){
		window.captureEvents(Event.CLICK);               
		window.onclick = message;
		function message(e) {
		updateSessionTimeout();
	//	checkSess();
		return true;
		}

		get_config(
				function(data)
				{
					change_locale('',data);
					check_usage(data);
					setUI();				
					if($(data).find("config").find("image").attr("invert") == "false")
					{	
						$(".lang_select_invert_image").html(_("NEGATIVE"));
					}
					else
					{
						$("#menu3_select_invert_image").find("span").addClass("menu3_image_invert");
						$(".lang_select_invert_image").html(_("POSITIVE"));
					}

					if(for_human == true)
					{	
						$("#menu3_select_usage").find("span").addClass("menu3_usage");
						$(".lang_select_usage").html(_("USING_FOR_VET"));
					}
					else
					{
						$(".lang_select_usage").html(_("USING_FOR_HUMAN"));
					}

					if(CheckInvalidAccess() == false)
						return;
				},
				function(data)
				{
					if(CheckInvalidAccess() == false)
						return;
					setUI();				
				});

});

function CheckInvalidAccess()
{

	var requiredfrom = "login.html"; // required prev. page 
	if (document.referrer.indexOf(requiredfrom) == -1) { 
		window.location="index.html"; 
		alert(_("INVALID_ACCESS"));
		return false;
	}
	return true;
}

function display_neutered()
{
	if( for_human == true )
	{
		$('#neutered').hide();
	}
	else
	{
		$('#neutered').empty();
		
		var neutered_select = document.getElementById("neutered");
		var option1 = document.createElement("option");
		option1.text = _("NEUTERED");
		option1.value = "";
		neutered_select.appendChild(option1);

		var option2 = document.createElement("option");
		option2.text = _("ALTERED");
		option2.value = "altered";
		neutered_select.appendChild(option2);

		var option3 = document.createElement("option");
		option3.text = _("UNALTERED");
		option3.value = "unaltered";
		neutered_select.appendChild(option3);
		
		//$('#neutered').val("altered");

		$('#neutered').show();
	}
}

function setUI()
{
		document.$_GET = [];      // 이부분이.... php문법이다.. get방식으로 받아온 값이 저장되는곳인듯. 
		var urlHalves = String(document.location).split('?');   // 현재 문서의 주소를 ?를 기준으로 나눔. www.xxx.com/index.html?var1=1212  였다면 www.xxx.com/index.html 과 var1=1212로 배열에 저장됨. 
		if(urlHalves[1])
		{      // get방식으로 넘어온 값이 있으면. 
			var urlVars = urlHalves[1].split('&');      // 값이 하나가 아닐수도 있으니깐. get방식에서 변수 여러개는 &로 묶여서 온다.
			for(var i=0; i<=(urlVars.length); i++)
			{         
				if(urlVars[i])
				{            
					var urlVarPair = urlVars[i].split('=');            
					document.$_GET[urlVarPair[0]] = urlVarPair[1];
				//	alert(document.$_GET[urlVarPair[0]]);
					
					if(urlVarPair[0] == "type")
					{
						user = document.$_GET[urlVarPair[0]];
						if(user == "1")
						{
							var str = "<li id='menu3_reset_userpassword'>";
							str += "<h4>" + _("RESET_USER_PASSWORD") +"</h4></li>";
						//	$("#menu3_reset_userpassword").removeClass("hidden");
							$("#menu3_main > ul").append(str);
						}
					}
				}      
			}   
		}


			init();
			$('.nav_l').click(function(){
				//checkSess();

				var selected = $(this).attr("class").substring(10,11);
				$(this).parent().find("li").each(function(){
					var pos = $(this).attr("class").substring(10,11);
					if(pos == selected)
					{
					/*
						$(this).addClass("click");
						if(selected == 1)
							$("#menu2").addClass("show");
						else if(selected == 2)
							$("#menu1").addClass("show");
						else
							$("#menu" + selected).addClass("show");

						if(pos=="1")
						{
							getPatientListFromDetector(function(){});
							displayPatientList();
							$(".sub_nav").addClass("show");
						}
						else 
							$(".sub_nav").removeClass("show");
							*/
						clickNav(pos,"on")
					}
					else
					{
						/*
						$(this).removeClass("click");
						if(pos == 1)
							$("#menu2").removeClass("show");
						else if(pos == 2)
							$("#menu1").removeClass("show");
						else 
							$("#menu" + pos).removeClass("show");
							*/
						clickNav(pos,"off");
					}
					});
			});

			$('.sub_nav_l').click(function(){
					if($(this).hasClass("click"))
						return;

				var selected = $(this).attr("class").substring(18,19);
				$(this).parent().find("li").each(function(){
					var pos = $(this).attr("class").substring(18,19);
					if(pos == selected)
					{
						$(this).addClass("click");
						$(".menu2_2_3").removeClass("show");
						
						if( selected == 2 )
						{
							if( for_human == false )
							{
								$(".menu2_2").removeClass("show");
								$(".menu2_2_3").addClass("show");
							}
							else
							{
								$(".menu2_2_3").removeClass("show");
								$(".menu2_2").addClass("show");
							}
						}
						else
						{
							$(".menu2_" + selected).addClass("show");
						}
						
						if(pos == "1")
						{
							view("loading");
							getPatientListFromDetector(function(data){
								displayPatientList();
								hide("loading");
								});
						}
						else if(pos == "2")
						{
							$('#menu2_2_name').val("");
							$('#menu2_2_birth').val("");
							$('#menu2_2_id').val("");
							$('#menu2_2_accno').val("");
							$('#menu2_2_etc').val("");
							$('#menu2_2_3_name').val("");
							$('#menu2_2_3_birth').val("");
							$('#menu2_2_3_id').val("");
							$('#menu2_2_3_accno').val("");
							$('#menu2_2_3_etc').val("");
							$('#menu2_2_3_species').val("");
							$('#menu2_2_3_breed').val("");
							$('#menu2_2_3_responsible').val("");
							display_neutered();
							
							removeCheckedAll();
						}
						else if(pos == "3")
						{
							setEmergency();
							displayPatientInfoToHTML();	
							clickNav(1,"off")
							clickNav(2,"on")
						}


					}
					else
					{
						$(this).removeClass("click");
						$(".menu2_" + pos).removeClass("show");
					}
					});
			});

			$("#menu1_txt").click(function(){
					var str="";
					str+= _("PATIENT_INFO");
					str+= "\n" +_("PATIENT_ID") + " : " + $("#menu1_txt_2").text();
					str+= "\n" + _("NAME") + " : " + $("#menu1_txt_1").text();
					str+= "\n" + _("BIRTH") + ": " + $("#menu1_txt_5").text() ;
					str+= "\n" + _("ACCNO") + " : " + $("#menu1_txt_3").text();
					str+= "\n" + _("STUDY_TIME") + " : " + $("#menu1_txt_4").text();
					str+= "\n\n" + _("STEP_INFO");
					str+= "\n" + _("BODYPART") + " : " + $("#menu1_txt_6").text();
					str+= "\n" + _("PROJECTION") + " : " + $("#menu1_txt_7").text();
					alert(str);
					
					});

			$("#menu3_delete_patientinfo").click(function(){
					var ret = confirm(_("DELETE_ALL_STUDY_CONFIRM"));
					if(ret == true)
						deleteAllPatientInfo();
						view("loading");
						sleep(1000); // 
						getPatientListFromDetector(function(data){
							hide("loading");
							displayPatientList();
							});
						sleep(1000); // 
					});

			$("#menu3_refresh_xml").click(function(){
						view("loading");
						getStepFromDetector();
						getPatientListFromDetector(function(data){
							hide("loading");
							displayPatientList();
							});
						alert(_("REFRESH_END"));
					});

			$("#menu3_ap_onoff").click(function(){
						var ret = confirm(_("AP_CONFIRM"));
						if(ret == true)
						{
							var span = $(this).find("span");
							var value;
							if($(span).hasClass("menu3_ap_on"))
							{
								$(span).addClass("menu3_ap_off");
								$(span).removeClass("menu3_ap_on");
								$(span).html(_("TURN_OFF_AP_MODE"));
								value = "off";
							}
							else if($(span).hasClass("menu3_ap_off"))
							{
								$(span).addClass("menu3_ap_on");
								$(span).removeClass("menu3_ap_off");
								$(span).html(_("TURN_ON_AP_MODE"));
								value = "on";
							}
							setConfig("AP","ap_onoff",value,function(){}, function(){});
							//window.location = "login.html";
						}
					});

			$("#menu3_change_password").click(function(){
						var str="";
						str += "<div class='menu3_sub_changepassword'>";
						str += "<ul class='menu3_sub_u'>";
						str += "<li class='menu3_sub_l'>" + _("CHANGE_PASSWORD") + "</li>";
						str += "<li class='menu3_sub_l'><input type='password' name='password' title='Password' value=''  autofocus='autofocus' id='menu3_sub_password' class='menu3_sub_input' placeholder='" + _("PASSWORD") +  "'  maxlength='16'></input></li>";
						str += "<li class='menu3_sub_l'><input type='password' name='new password' title='New Password' value='' id='menu3_sub_newpassword' class='menu3_sub_input' placeholder='" + _("NEW_PASSWORD") + "'  maxlength='16'></input></li>";
						str += "<li class='menu3_sub_l'><input type='password' name='new password again' title='New Password again' value='' id='menu3_sub_newpasswordagain' class='menu3_sub_input' placeholder='" + _("AGAIN_NEW_PASSWORD") + "'  maxlength='16'></input></li>";
						str += "</ul>";
						str += "<button type='button' title='Change password' class='menu3_sub_btn btn' id='menu3_sub_ok' onclick='bclk(this,\"10\");'>" + _("OK") + " </button>";
						str += "<button type='button' title='Change password' class='menu3_sub_btn btn' id='menu3_sub_cancel' onclick='bclk(this,\"10\");'>" + _("CANCEL") + "</button>";
						str += "</div>";

						menu3_sub(true, str);
					});

			$("#menu3_reset_userpassword").click(function(){
					var ret = confirm(_("RESET_USER_PASSWORD_CONFIRM"));
					if(ret == true)
					{
						initPasswd(2 , function(data){
							var ret = checkReturnValue(data,"VALUE");
							if(ret == 0)
								alert(_("PASSWORD_INITIALIZE_SUCCESS"));
							else 
								alert(_("PASSWORD_INITIALIZE_FAIL"));
							},function(data){
								alert(_("PASSWORD_INITIALIZE_FAIL"));

							});
						}

					});

			$("#menu3_select_invert_image").click(function(){
					var span = $(this).find("span");
					var value = "false";
					if($(span).hasClass("menu3_image_invert"))
					{
						$(span).removeClass("menu3_image_invert");
						$(span).html(_("NEGATIVE"));
					}
					else
					{
						$(span).addClass("menu3_image_invert");
						$(span).html(_("POSITIVE"));
						value = "true";
					}
					invertImage(value, function(){}, function(){});

					});

			$("#menu3_select_usage").click(function(){
					var span = $(this).find("span");
					var value = "false";
					if($(span).hasClass("menu3_usage"))
					{
						$(span).removeClass("menu3_usage");
						$(span).html(_("USING_FOR_HUMAN"));
						for_human = false;
					}
					else
					{
						$(span).addClass("menu3_usage");
						$(span).html(_("USING_FOR_VET"));
						for_human = true;
						value = "true";
					}
					use_for_human(value, function(){}, function(){});

					});


			$(".loading").click(function(){
						if($(this).hasClass("loading_exposure"))
						{
						var ret = confirm(_("CANCEL_EXPOSURE_CONFIRM"));
						if(ret == true)
						{
						clearTimeout(timer);
						timer = -1;
						strHtml="<center><p><h3>" + _("EXPOSURE_CANCEL") + "</h3></p></center>"
						$("#menu1_img").html(strHtml);
						hide("loading");
						exposure("off");

							
						}
						}

						});

			$(".lang_id").attr("placeholder",_("PATIENT_ID"));
			$(".lang_name").attr("placeholder",_("NAME"));
			$(".lang_date_of_birth").attr("placeholder",_("DATE_OF_BIRTH") + "(YYYY-MM-DD)");
			$(".lang_accno").attr("placeholder",_("ACCNO"));
			$(".lang_info").attr("placeholder",_("INFO"));

			$("#status_l_logout_btn").html(_("LOGOUT"));

			$(".lang_worklist").html(_("WORKLIST"));
			$(".lang_registration").html(_("REGISTRATION"));
			$(".lang_accno").html(_("ACCNO"));
			$(".lang_birth").html(_("BIRTH"));
			$(".lang_exposure").html(_("EXPOSURE"));
			$(".lang_setting").html(_("SETTING"));
			$(".lang_list").html(_("LIST"));
			$(".lang_emergency").html(_("EMERGENCY"));

			$(".lang_patient_info").html(_("PATIENT_INFO"));
			$(".lang_step_info").html(_("STEP_INFO"));
			$(".lang_bodypart").html(_("BODYPART"));
			$(".lang_projection").html(_("PROJECTION"));
			$(".lang_patinet_id").html(_("PATIENT_ID"));
			$(".lang_name").html(_("NAME"));
			$(".lang_study_date").html(_("STUDY_TIME"));
			
			$(".lang_neutered").html(_("NEUTERED"));
			$(".lang_species").attr("placeholder",_("SPECIES_DESCRIPTION"));
			$(".lang_species").html(_("SPECIES_DESCRIPTION"));
			$(".lang_breed").attr("placeholder",_("BREED_DESCRIPTION"));
			$(".lang_breed").html(_("BREED_DESCRIPTION"));
			$(".lang_responsible").attr("placeholder",_("RESPONSIBLE_PERSON"));
			$(".lang_responsible").html(_("RESPONSIBLE_PERSON"));
				
			$(".lang_add_bodypart").html(_("ADD_BODYPART"));
			$(".lang_new_study").html(_("NEW_STUDY"));
			$(".lang_caution").html(_("RESTRICTION_CHARACTER"));

			$(".lang_delete_patient_info").html(_("DELETE_ALL_STUDY"));
			$(".lang_refresh").html(_("REFRESH"));
			$(".lang_ap").html(_("TURN_ON_AP_MODE"));
			$(".lang_change_passwd").html(_("CHANGE_PASSWORD"));
			$(".lang_select_invert_image").html(_("IMAGE INVERT"));
			$(".lang_select_usage").html(_("USE_FOR_HUMAN"));
}

function ConfigSetting()
{
	getConfig(
			function(data)
			{
				if( $(data).find("DEVICE").length > 0)
				{
					var str="";
					
					var onoff = $(data).find("DEVICE").find("AP").attr("ap_onoff");
					str = "menu3_ap_" + onoff;
					$("#menu3_ap_onoff").find("span").removeClass("menu3_ap_on menu3_ap_off").addClass(str);
					str = (onoff == "on") ? _("TURN_OFF_AP_MODE") : _("TURN_ON_AP_MODE");
					$("#menu3_ap_onoff").find("span").html(str);
					
				}
			},
		function(data)
		{

			});

}

function scroll() {
	var scroll, i,
		positions = [],
		here = $(window).scrollTop(),
		collection = $('#menu1_img');
	collection.each(function() {
			positions.push(parseInt($(this).offset()['top'],10));
			});

	scroll = collection.get(0);
/*
	for(i = 0; i < positions.length; i++) {
		if (direction == 'next' && positions[i] > here) 
		{ scroll = collection.get(i); break; }
		if (direction == 'prev' && i > 0 && positions[i] >= here) 
		{ scroll = collection.get(i-1); break; }
	}
	*/
	if (scroll) {
		$.scrollTo(scroll, {
duration: 750
});
}
return false;
}

function checkSess()
{
	checkSession(function(data)
			{
				var ret = checkReturnValue(data,"VALUE");
				if(ret!=0x00)
				{
					window.location = "login.html";
					alert(_("RE_LOGIN"));
				}
				
			},
			function(data)
			{
				if(logoutFlag == false)
				{
					logoutFlag = true;
					clearTimeout(StatusTimer);
					window.location = "login.html";
					alert(_("CHECK_NETWORK_STATUS"));
				}
			}
			);

}

function updateSessionTimeout()
{
	updateTimeout(function(data)
			{
			},
			function(data)
			{
			}
			);

}
function exposure(data)
{
	if(data == "on")
	{
		$("#loading").addClass("loading_exposure")
	}
	else if(data == "off")
	{
		$("#loading").removeClass("loading_exposure")
	}
}

