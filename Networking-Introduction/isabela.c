#include "header.h"

/*
This example project use the Json-C library to decode the objects to C char arrays and
uses the C libcurl library to request the data to the API.
*/


struct string {
	char *ptr;
	size_t len;
};


//Write function to write the payload response in the string structure
size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
	size_t new_len = s->len + size*nmemb;
	s->ptr = realloc(s->ptr, new_len + 1);
	if (s->ptr == NULL) {
		fprintf(stderr, "realloc() failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(s->ptr + s->len, ptr, size*nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;

	return size*nmemb;
}

//Initilize the payload string
void init_string(struct string *s) {
	s->len = 0;
	s->ptr = malloc(s->len + 1);
	if (s->ptr == NULL) {
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	s->ptr[0] = '\0';
}

//Get the Data from the API and return a JSON Object
struct json_object *get_student_data()
{
	struct string s;
	struct json_object *jobj;

	//Intialize the CURL request
	CURL *hnd = curl_easy_init();

	//Initilize the char array (string)
	init_string(&s);

	curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "GET");
	//To run on department network uncomment this request and comment the other
	//curl_easy_setopt(hnd, CURLOPT_URL, "http://10.3.4.75:9014/v2/entities?options=keyValues&type=student&attrs=activity,calls_duration,calls_made,calls_missed,calls_received,department,location,sms_received,sms_sent&limit=1000");
        //To run from outside
	curl_easy_setopt(hnd, CURLOPT_URL, "http://socialiteorion2.dei.uc.pt:9014/v2/entities?options=keyValues&type=student&attrs=activity,calls_duration,calls_made,calls_missed,calls_received,department,location,sms_received,sms_sent&limit=1000");

	//Add headers
	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "cache-control: no-cache");
	headers = curl_slist_append(headers, "fiware-servicepath: /");
	headers = curl_slist_append(headers, "fiware-service: socialite");

	//Set some options
	curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, writefunc); //Give the write function here
	curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &s); //Give the char array address here

	//Perform the request
	CURLcode ret = curl_easy_perform(hnd);
	if (ret != CURLE_OK){
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(ret));

		/*jobj will return empty object*/
		jobj = json_tokener_parse(s.ptr);

		/* always cleanup */
		curl_easy_cleanup(hnd);
		return jobj;

	}
	else if (CURLE_OK == ret) {
		jobj = json_tokener_parse(s.ptr);
		free(s.ptr);

		/* always cleanup */
		curl_easy_cleanup(hnd);
		return jobj;
	}
}


int get_data(ClientData *data)
{
	//JSON obect
	struct json_object *jobj_array, *jobj_obj;
	struct json_object *jobj_object_id, *jobj_object_type, *jobj_object_activity, *jobj_object_location, *jobj_object_latlong, *jobj_object_callsduration,
	*jobj_object_callsmade, *jobj_object_callsmissed, *jobj_object_callsreceived, *jobj_object_department, *jobj_object_smsreceived, *jobj_object_smssent;
	enum json_type type = 0;
	int arraylen = 0;
	int i;

	ClientData *tmp;

	//Get the student data
	jobj_array = get_student_data();

	//Get array length
	arraylen = json_object_array_length(jobj_array);


	//Example of howto retrieve the data
	for (i = 0; i < arraylen; i++) {
		//get the i-th object in jobj_array
		jobj_obj = json_object_array_get_idx(jobj_array, i);

		//get the name attribute in the i-th object
		jobj_object_id = json_object_object_get(jobj_obj, "id");
		jobj_object_type = json_object_object_get(jobj_obj, "type");
		jobj_object_activity = json_object_object_get(jobj_obj, "activity");
		jobj_object_location = json_object_object_get(jobj_obj, "location");
		jobj_object_callsduration = json_object_object_get(jobj_obj, "calls_duration");
		jobj_object_callsmade = json_object_object_get(jobj_obj, "calls_made");
		jobj_object_callsmissed = json_object_object_get(jobj_obj, "calls_missed");
		jobj_object_callsreceived= json_object_object_get(jobj_obj, "calls_received");
		jobj_object_department = json_object_object_get(jobj_obj, "department");
		jobj_object_smsreceived = json_object_object_get(jobj_obj, "sms_received");
		jobj_object_smssent = json_object_object_get(jobj_obj, "sms_sent");

		/*	MY CODE	*/
		if (!(tmp = (ClientData *)malloc(sizeof(ClientData))))
		{
			printf("Error on malloc()\n");
			return -1;
		}

		//	Fill the structure tmp
		strcpy(tmp->id, json_object_get_string(jobj_object_id));

		if (json_object_get_string(jobj_object_type))
			strcpy(tmp->type, json_object_get_string(jobj_object_type));
		else
			strcpy(tmp->type, "NULL");

		if (json_object_get_string(jobj_object_activity))
			strcpy(tmp->activity, json_object_get_string(jobj_object_activity));
		else
			strcpy(tmp->activity, "NULL");

		if (json_object_get_string(jobj_object_location))
			strcpy(tmp->location, json_object_get_string(jobj_object_location));
		else
			strcpy(tmp->location, "NULL");

		if (json_object_get_string(jobj_object_department))
			strcpy(tmp->department, json_object_get_string(jobj_object_department));
		else
			strcpy(tmp->department, "NULL");

		if (json_object_get_string(jobj_object_callsduration))
			sscanf(json_object_get_string(jobj_object_callsduration), "%lf", &tmp->calls_duration);
		else
			tmp->calls_duration = 0.0;

		if (json_object_get_string(jobj_object_callsmade))
			sscanf(json_object_get_string(jobj_object_callsmade), "%lf", &tmp->calls_made);
		else
			tmp->calls_made = 0.0;

		if (json_object_get_string(jobj_object_callsmissed))
			sscanf(json_object_get_string(jobj_object_callsmissed), "%lf", &tmp->calls_missed);
		else
			tmp->calls_missed = 0.0;

		if (json_object_get_string(jobj_object_callsreceived))
			sscanf(json_object_get_string(jobj_object_callsreceived), "%lf", &tmp->calls_rcv);
		else
			tmp->calls_rcv = 0.0;

		if (json_object_get_string(jobj_object_smsreceived))
			sscanf(json_object_get_string(jobj_object_smsreceived), "%lf", &tmp->sms_rcv);
		else
			tmp->sms_rcv = 0.0;

		if (json_object_get_string(jobj_object_smssent))
			sscanf(json_object_get_string(jobj_object_smssent), "%lf", &tmp->sms_sent);
		else
			tmp->sms_sent = 0.0;

		//	Add new data to list
		tmp->next = data->next;
		data->next = tmp;

		// The first object of the 'data' list is for the group data
		//	We initialize it here
		data->calls_duration += tmp->calls_duration;
		data->calls_made += tmp->calls_made;
		data->calls_missed += tmp->calls_missed;
		data->calls_rcv += tmp->calls_rcv;
		data->sms_rcv += tmp->sms_rcv;
		data->sms_sent += tmp->sms_sent;
	}

	// Calculate the average
	data->calls_duration /= i;
	data->calls_made /= i;
	data->calls_missed /= i;
	data->calls_rcv /= i;
	data->sms_rcv /= i;
	data->sms_sent /= i;

	return 0;
}


void refresh_data(struct aux *aux)
{
	struct json_object *jobj_array, *jobj_obj;
	struct json_object *jobj_object_id, *jobj_object_type, *jobj_object_activity, *jobj_object_location, *jobj_object_latlong, *jobj_object_callsduration,
	*jobj_object_callsmade, *jobj_object_callsmissed, *jobj_object_callsreceived, *jobj_object_department, *jobj_object_smsreceived, *jobj_object_smssent;
	enum json_type type = 0;
	int arraylen = 0;
	int i;

	ClientData *tmp;
	double cd, cma, cmi, cr, mr, ms;
	int id_already_existed;

	cd = cma = cmi = cr = mr = ms = 0.0;

	//Get the student data
	jobj_array = get_student_data();

	//Get array length
	arraylen = json_object_array_length(jobj_array);


	//Example of howto retrieve the data
	for (i = 0; i < arraylen; i++) {
		//get the i-th object in jobj_array
		jobj_obj = json_object_array_get_idx(jobj_array, i);

		//get the name attribute in the i-th object
		jobj_object_id = json_object_object_get(jobj_obj, "id");
		jobj_object_type = json_object_object_get(jobj_obj, "type");
		jobj_object_activity = json_object_object_get(jobj_obj, "activity");
		jobj_object_location = json_object_object_get(jobj_obj, "location");
		jobj_object_callsduration = json_object_object_get(jobj_obj, "calls_duration");
		jobj_object_callsmade = json_object_object_get(jobj_obj, "calls_made");
		jobj_object_callsmissed = json_object_object_get(jobj_obj, "calls_missed");
		jobj_object_callsreceived= json_object_object_get(jobj_obj, "calls_received");
		jobj_object_department = json_object_object_get(jobj_obj, "department");
		jobj_object_smsreceived = json_object_object_get(jobj_obj, "sms_received");
		jobj_object_smssent = json_object_object_get(jobj_obj, "sms_sent");

		/*	Search if this client already exists	*/
		id_already_existed = 1;
		if (!(tmp = is_id_in(json_object_get_string(jobj_object_id), aux->data->next)))
		{
			if (!(tmp = (ClientData *)malloc(sizeof(ClientData))))
			{
				//printf("Error on malloc()\n");
				continue;
			}
			id_already_existed = 0;
		}

		//	Fill the structure tmp
		strcpy(tmp->id, json_object_get_string(jobj_object_id));

		if (json_object_get_string(jobj_object_type))
			strcpy(tmp->type, json_object_get_string(jobj_object_type));
		else
			strcpy(tmp->type, "NULL");

		if (json_object_get_string(jobj_object_activity))
			strcpy(tmp->activity, json_object_get_string(jobj_object_activity));
		else
			strcpy(tmp->activity, "NULL");

		if (json_object_get_string(jobj_object_location))
			strcpy(tmp->location, json_object_get_string(jobj_object_location));
		else
			strcpy(tmp->location, "NULL");

		if (json_object_get_string(jobj_object_department))
			strcpy(tmp->department, json_object_get_string(jobj_object_department));
		else
			strcpy(tmp->department, "NULL");

		if (json_object_get_string(jobj_object_callsduration))
			sscanf(json_object_get_string(jobj_object_callsduration), "%lf", &tmp->calls_duration);
		else
			tmp->calls_duration = 0.0;

		if (json_object_get_string(jobj_object_callsmade))
			sscanf(json_object_get_string(jobj_object_callsmade), "%lf", &tmp->calls_made);
		else
			tmp->calls_made = 0.0;

		if (json_object_get_string(jobj_object_callsmissed))
			sscanf(json_object_get_string(jobj_object_callsmissed), "%lf", &tmp->calls_missed);
		else
			tmp->calls_missed = 0.0;

		if (json_object_get_string(jobj_object_callsreceived))
			sscanf(json_object_get_string(jobj_object_callsreceived), "%lf", &tmp->calls_rcv);
		else
			tmp->calls_rcv = 0.0;

		if (json_object_get_string(jobj_object_smsreceived))
			sscanf(json_object_get_string(jobj_object_smsreceived), "%lf", &tmp->sms_rcv);
		else
			tmp->sms_rcv = 0.0;

		if (json_object_get_string(jobj_object_smssent))
			sscanf(json_object_get_string(jobj_object_smssent), "%lf", &tmp->sms_sent);
		else
			tmp->sms_sent = 0.0;


		/*	IF this is a new client THEN	*/
		if (!id_already_existed)
		{
			//	Add new data to list
			tmp->next = aux->data->next;
			aux->data->next = tmp;
		}

		cd += tmp->calls_duration;
		cma += tmp->calls_made;
		cmi += tmp->calls_missed;
		cr += tmp->calls_rcv;
		mr += tmp->sms_rcv;
		ms += tmp->sms_sent;
	}

	cd /= i; cma /= i; cmi /= i; cr /= i; mr /= i; ms /= i;

	/*	Check if any change happenned and if it did: tell the subscribers	*/
	id_already_existed = 0;

	if (cd != aux->data->calls_duration)
	{
		tell_subscribers(aux->subs_list, "cd");
		id_already_existed = 1;
	}
	if (cma != aux->data->calls_made)
	{
		tell_subscribers(aux->subs_list, "cma");
		id_already_existed = 1;
	}
	if (cmi != aux->data->calls_missed)
	{
		tell_subscribers(aux->subs_list, "cmi");
		id_already_existed = 1;
	}
	if (cr != aux->data->calls_rcv)
	{
		tell_subscribers(aux->subs_list, "cr");
		id_already_existed = 1;
	}
	if (mr != aux->data->sms_rcv)
	{
		tell_subscribers(aux->subs_list, "mr");
		id_already_existed = 1;
	}
	if (ms != aux->data->sms_sent)
	{
		tell_subscribers(aux->subs_list, "ms");
		id_already_existed = 1;
	}

	/*	A change was made, notify who subscribed to ALL the group data	*/
	if (id_already_existed)
		tell_subscribers(aux->subs_list, "all");


	/*	Save the new averages	*/
	aux->data->calls_duration = cd;
	aux->data->calls_made = cma;
	aux->data->calls_missed = cmi;
	aux->data->calls_rcv = cr;
	aux->data->sms_rcv = mr;
	aux->data->sms_sent = ms;


	return;
}
