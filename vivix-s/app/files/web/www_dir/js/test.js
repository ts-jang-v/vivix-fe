var loading = false;

function TEST_success(data)
{
	var ret;
	ret = checkReturnValue(data,"ETC"); 
	document.getElementById("result").innerHTML= ret;
}

function TEST_error(data)
{
	alert("error");
}
	

function TEST_sendHttpRequest()
{
	var data = new HttpRequestData();
	data.type = "get";
	data.url = "./xml/response.xml";
	data.dataType = "xml";
	data.cache="false";
	data.cmdtype = 02;
	data.count = 1;
	data.userdata = "0102";


	sendHttpRequest(data, TEST_success, TEST_error);

	delete(data);
}

function TEST_sendPatientInfo()
{
	sendInformation(function(){
				setTimeout("pollImage(" + checkReturnValue(data,"VALUE")
+ ")",1000);
							
			},
			function(){

			});

}

function pollImage(filename)
{

	var strHtml = "<img src='../img/" + filename + "?time="+ new Date() + "' width='100%'></img>";
	$("#SI_1_image_div").html( strHtml);

}


function TEST_login(id, passwd)
{
	login(id,passwd, 
			function(data){
			printDOMTree(data);
				document.getElementById("result").innerHTML= 
				"rsponse = " + checkReturnValue(data,"VALUE") +
				", " + checkReturnValue(data,"ETC");

			}, 
			function(data){
			printDOMTree(data);
				document.getElementById("result").innerHTML= "error";


			});
}

function TEST_changePasswd(passwd, newpasswd)
{
	changePasswd(passwd,newpasswd, 
			function(data){
				
				document.getElementById("result").innerHTML= 
				"response = " + checkReturnValue(data,"VALUE") +
				", " + checkReturnValue(data,"ETC");

			}, 
			function(data){
				document.getElementById("result").innerHTML= "error";

			});
}

function TEST_getCurrentPatient()
{
	getCurrentPatient( 
			function(data){
			
				g_currentPatient = data;
				var info = $(data).find("patient").find("info");
				var step = $(data).find("patient").find("step");

				var str ="name : " + info.attr("name");
				str +="<br>id : " + info.attr("id");
				str +="<br>accno : " + info.attr("accno");
				str +="<br>birth : " + info.attr("birth");
				str +="<br>step info : " + step.attr("key") + ", " + step.attr("bodypart") + ", " + step.attr("projection");
				document.getElementById("result").innerHTML= str;

			}, 
			function(data){
				document.getElementById("result").innerHTML= "error";

			});
}

function TEST_setPatientInfo()
{
	setPatientInfo(2);
	TEST_displayPatientInfo();
}

function TEST_displayPatientInfo()
{
	data = g_currentPatient;
	var info = $(data).find("patient").find("info");
	var step = $(data).find("patient").find("step");

	var str ="name : " + info.attr("name");
	str +="<br>id : " + info.attr("id");
	str +="<br>accno : " + info.attr("accno");
	str +="<br>birth : " + info.attr("birth");
	str +="<br>step info : " + step.attr("key") + ", " + step.attr("bodypart") + ", " + step.attr("projection");
	document.getElementById("result").innerHTML= str;


}


function TEST_getStepInfoByKey(key)
{
	var info = getStepInfoByKey(g_stepList,key);
	var str ="bodypart: " + info[0];
	str +="<br>projection : " + info[1];
	str +="<br>key : " + key;
	document.getElementById("result").innerHTML= str;


}


function TEST_getPatientStepInfo(id)
{
	var stepList = getPatientStepInfo(id)
	var info;
	var list = g_patientList;
	$(list).find("patient").each(function(){
			var patient = $(this);
			if(patient.attr("id") == id)
			{
				$(patient).find("step").each(function(){
					var key = $(this).attr("key");
					info = getStepInfoByKey(stepList,key);
					var str ="bodypart: " + info[0];
					str +="<br>projection : " + info[1];
					str +="<br>key : " + key;
					document.getElementById("result").innerHTML= str;
//					alert(str);
					});

			}
			
			});

	printDOMTree(stepList);
}

function TEST_LoadingIcon()
{
	if(loading == true)
		hide("loading");
	else
		view("loading");

	loading = loading ? false : true;

}


function printDOMTree(domElement, destinationWindow)
{
	// Use destination window to print the tree.  If destinationWIndow is
	//   not specified, create a new window and print the tree into that window
	var outputWindow=destinationWindow;
	if (!outputWindow)
		outputWindow=window.open();

	// make a valid html page
	outputWindow.document.open("text/html", "replace");
	outputWindow.document.write("<HTML><HEAD><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0 minimum-scale=1.0 maximum-scale=1.0\"><TITLE>DOM</TITLE></HEAD><BODY>\n");
	outputWindow.document.write("<CODE>\n");
	traverseDOMTree(outputWindow.document, domElement, 1);
	outputWindow.document.write("</CODE>\n");
	outputWindow.document.write("</BODY></HTML>\n");

	// Here we must close the document object, otherwise Mozilla browsers 
	//   might keep showing "loading in progress" state.
	outputWindow.document.close();
}

function traverseDOMTree(targetDocument, currentElement, depth)
{
	if (currentElement)
	{
		var j;
		var tagName=currentElement.tagName;
		var attibutes=currentElement.attributes;
		// Prints the node tagName, such as <A>, <IMG>, etc
		if (tagName)
		{
			targetDocument.writeln("&lt;"+currentElement.tagName);
			var len = currentElement.attributes.length;
			
			if(len > 0 )
			{
				var i=0;
				for(i;i < len ; i++)
					targetDocument.writeln(currentElement.attributes[i].nodeName + "=\"" + currentElement.attributes[i].nodeValue + "\"");
			}
			targetDocument.writeln("&gt;");
		}
		else
			targetDocument.writeln("[unknown tag]");

		// Traverse the tree
		var i=0;
		var currentElementChild=currentElement.childNodes[i];
		while (currentElementChild)
		{
			// Formatting code (indent the tree so it looks nice on the screen)
			targetDocument.write("<BR>\n");
			for (j=0; j<depth; j++)
			{
				// &#166 is just a vertical line
				targetDocument.write("&nbsp;&nbsp;&#166");
			}								
			targetDocument.writeln("<BR>");
			for (j=0; j<depth; j++)
			{
				targetDocument.write("&nbsp;&nbsp;&#166");
			}					
			if (tagName)
				targetDocument.write("--");

			// Recursively traverse the tree structure of the child node
			traverseDOMTree(targetDocument, currentElementChild, depth+1);
			i++;
			currentElementChild=currentElement.childNodes[i];
		}
		// The remaining code is mostly for formatting the tree
		targetDocument.writeln("<BR>");
		for (j=0; j<depth-1; j++)
		{
			targetDocument.write("&nbsp;&nbsp;&#166");
		}			
		targetDocument.writeln("&nbsp;&nbsp;");
		if (tagName)
			targetDocument.writeln("&lt;/"+tagName + "&gt;");
	}
}

function TEST_addPatient()
{
	addPatient("kim","1974.1.11","P007","A3","");
	printDOMTree(g_patientList);
}


function TEST_addStep()
{
	addStepToPatient("P001", 7);
	printDOMTree(g_patientList);
}

function TEST_markExpEnd()
{
	markExpEnd(g_currentPatient);
	printDOMTree(g_patientList);

}
