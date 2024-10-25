var MSG = new Array(
	'TITLE',
	'ID',
	'PASSWORD',
	'LOGIN',
	'BATTERY',
	'LOGOUT',
	'BATTERY_LOW',
	'BATTERY_QUATER',
	'BATTERY_HALF',
	'BATTERY_THREE_QUATERS',
	'BATTERY_FULL',
	'WORKLIST',
	'REGISTRATION',
	'NAME',
	'ACCNO',
	'INFO',
	'ADD_BODYPART',
	'NEW_STUDY',
	'RESTRICTION_CHARACTER',
	'FAVORITE',
	'ALL',
	'OK',
	'EXPOSURE',
	'PATIENT_INFO',
	'PATIENT_ID',
	'STUDY_TIME',
	'STEP_INFO',
	'BODYPART',
	'PROJECTION',
	'EMERGENCY',
	'BODYPART_SELECTION',
	'SELECTED',
	'BODYPART_NOT_EXIST',
	'CANCEL',
	'SETTING',
	'DELETE_ALL_STUDY',
	'REFRESH',
	'TURN_OFF_AP_MODE',
	'TURN_ON_AP_MODE',
	'CHANGE_PASSWORD',
	'RESET_USER_PASSWORD',
	'DELETE_ALL_STUDY_CONFIRM',
	'REFRESH_END',
	'REBOOT_CONFIRM',
	'NEW_PASSWORD',
	'AGAIN_NEW_PASSWORD',
	'RESET_USER_PASSWORD_CONFIRM',
	'LIST',
	'INVALID_ACCESS',
	'INSERT_NAME',
	'INSERT_ID',
	'INSERT_PASSWORD',
	'INSERT_NEW_PASSWORD',
	'INSERT_NEW_PASSWORD_AGAIN',
	'DOES_NOT_MATCH_PASSWORD',
	'PASSWORD_CHANGE_SUCCESS',
	'CANNOT_READ_PASSWORD',
	'CHECK_PASSWORD',
	'CANNOT_CHANGE_PASSWORD',
	'CANNOT_WRITE_NEW_PASSWORD',
	'UNKNOWN_ERROR',
	'BIRTH',
	'ETC',
	'SPECIES_DESCRIPTION',
	'BREED_DESCRIPTION',
	'RESPONSIBLE_PERSON',
	'NEUTERED',
	'ALTERED',
	'UNALTERED',
	'USE_FOR_HUMAN',
	'USE_FOR_VET',
	'COMPLETE',
	'DELETE_SELECTED_STUDY_CONFIRM',
	'IMAGE_DOES_NOT_MATCH_PATIENT_INFO',
	'OTHER_DEVICES',
	'GOOD_BYE',
	'PASSWORD_INITIALIZE_SUCCESS',
	'PASSWORD_INITIALIZE_FAIL',
	'CANCEL_EXPOSURE_CONFIRM',
	'EXPOSURE_CANCEL',
	'RE_LOGIN',
	'CHECK_NETWORK_STATUS',
	'NONE',
	'LOGIN_ERROR',
	'RESET_PASSWORD',
	'CHECK_ID_PASSWORD',
	'ANOTHER_USER_HAS_LOGIN_ALREADY',
	'ANOTHER_ID_HAS_LOGIN_ALREADY'
	);

function printAll(data, board)
{
	
	var str="<table border=1>";
	str += "<tr><td>Message</td><td>Before translation</td><td>After translation</td></tr>";	
	for(var i=0; i<MSG.length; i++)
	{
		str += "<tr>";	
		str += "<td>";	
		str += MSG[i];	
		str += "</td>";	
		delete(gt);
		gt = new Gettext({ domain : 'locale', locale_data : get_locale_data(LANG.US)});
		str += "<td>";	
		str += _(MSG[i]);	
		str += "</td>";	
		delete(gt);
		gt = new Gettext({ domain : 'locale', locale_data : get_locale_data(data)});
		str += "<td>";	
		str += _(MSG[i]);	
		str += "</td>";	
		str += "</tr>";	
	}
	str+="</table>";
	(board).html(str);
}
