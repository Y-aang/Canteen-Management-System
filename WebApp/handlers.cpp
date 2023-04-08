#include "handlers.h"

#include <vector>

#include "rendering.h"

// register an orm mapping (to convert the db query results into
// json objects).
// the db query results contain several rows, each has a number of
// fields. the order of `make_db_field<Type[i]>(name[i])` in the
// initializer list corresponds to these fields (`Type[0]` and
// `name[0]` correspond to field[0], `Type[1]` and `name[1]`
// correspond to field[1], ...). `Type[i]` is the type you want
// to convert the field value to, and `name[i]` is the identifier
// with which you want to store the field in the json object, so
// if the returned json object is `obj`, `obj[name[i]]` will have
// the type of `Type[i]` and store the value of field[i].
bserv::db_relation_to_object orm_user{
	bserv::make_db_field<int>("id"),
	bserv::make_db_field<std::string>("username"),
	bserv::make_db_field<std::string>("password"),
	bserv::make_db_field<bool>("is_superuser"),
	bserv::make_db_field<std::string>("first_name"),
	bserv::make_db_field<std::string>("last_name"),
	bserv::make_db_field<std::string>("email"),
	bserv::make_db_field<bool>("is_active")
};

bserv::db_relation_to_object orm_canteen{
	bserv::make_db_field<int>("C_"),
	bserv::make_db_field<std::string>("Cname"),
	bserv::make_db_field<std::string>("Cpicture")
};

bserv::db_relation_to_object orm_win{
	bserv::make_db_field<int>("W_"),
	bserv::make_db_field<std::string>("Wname"),
	bserv::make_db_field<std::string>("Wlocation"),
	bserv::make_db_field<int>("C_")
};

bserv::db_relation_to_object orm_tag{
	bserv::make_db_field<int>("T_"),
	bserv::make_db_field<std::string>("Tname"),
	bserv::make_db_field<int>("Tsupport")
};

bserv::db_relation_to_object orm_dish{
	bserv::make_db_field<int>("D_"),
	bserv::make_db_field<std::string>("Dname"),
	bserv::make_db_field<double>("Dprice"),
	bserv::make_db_field<bool>("is_sell"),
	bserv::make_db_field<std::string>("Dpicture"),
	bserv::make_db_field<int>("W_")
};

bserv::db_relation_to_object orm_remark{
	bserv::make_db_field<int>("R_"),
	bserv::make_db_field<std::string>("Rcontext"),
	bserv::make_db_field<int>("Rmark"),
	bserv::make_db_field<int>("id"),
	bserv::make_db_field<std::string>("username"),
	bserv::make_db_field<int>("D_")
};

bserv::db_relation_to_object orm_score{
	bserv::make_db_field<int>("score"),
};

bserv::db_relation_to_object orm_window_management{
	bserv::make_db_field<int>("W_"),
	bserv::make_db_field<std::string>("Wname"),
	bserv::make_db_field<std::string>("Wlocation"),
	bserv::make_db_field<int>("C_"),
	bserv::make_db_field<std::string>("Cname"),
	bserv::make_db_field<std::string>("Cpicture")
};

bserv::db_relation_to_object orm_dish_management{
	bserv::make_db_field<int>("D_"),
	bserv::make_db_field<std::string>("Dname"),
	bserv::make_db_field<double>("Dprice"),
	bserv::make_db_field<bool>("is_sell"),
	bserv::make_db_field<std::string>("Dpicture"),
	bserv::make_db_field<int>("W_"),
	bserv::make_db_field<std::string>("Wname"),
	bserv::make_db_field<std::string>("Wlocation"),
	bserv::make_db_field<int>("C_"),
	bserv::make_db_field<std::string>("Cname"),
	bserv::make_db_field<std::string>("Cpicture")
};

bserv::db_relation_to_object orm_tag_management{
	bserv::make_db_field<int>("T_"),
	bserv::make_db_field<std::string>("Tname")
};

std::optional<boost::json::object> get_user(
	bserv::db_transaction& tx,
	const boost::json::string& username) {
	bserv::db_result r = tx.exec(
		"select * from auth_user where username = ?", username);
	lginfo << r.query(); // this is how you log info
	return orm_user.convert_to_optional(r);
}

std::string get_or_empty(
	boost::json::object& obj,
	const std::string& key) {
	return obj.count(key) ? obj[key].as_string().c_str() : "";
}

// if you want to manually modify the response,
// the return type should be `std::nullopt_t`,
// and the return value should be `std::nullopt`.
std::nullopt_t hello(
	bserv::response_type& response,
	std::shared_ptr<bserv::session_type> session_ptr) {
	bserv::session_type& session = *session_ptr;
	boost::json::object obj;
	if (session.count("user")) {
		// NOTE: modifications to sessions must be performed
		// BEFORE referencing objects in them. this is because
		// modifications might invalidate referenced objects.
		// in this example, "count" might be added to `session`,
		// which should be performed first.
		// then `user` can be referenced safely.
		if (!session.count("count")) {
			session["count"] = 0;
		}
		auto& user = session["user"].as_object();
		session["count"] = session["count"].as_int64() + 1;
		obj = {
			{"welcome", user["username"]},
			{"count", session["count"]}
		};
	}
	else {
		obj = { {"msg", "hello, world!"} };
	}
	// the response body is a string,
	// so the `obj` should be serialized
	response.body() = boost::json::serialize(obj);
	response.prepare_payload(); // this line is important!
	return std::nullopt;
}

//登录用的函数
// if you return a json object, the serialization
// is performed automatically.
boost::json::object user_register(
	bserv::request_type& request,
	// the json object is obtained from the request body,
	// as well as the url parameters
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn) {
	if (request.method() != boost::beast::http::verb::post) {
		throw bserv::url_not_found_exception{};
	}
	if (params.count("username") == 0) {
		return {
			{"success", false},
			{"message", "`username` is required"}
		};
	}
	if (params.count("password") == 0) {
		return {
			{"success", false},
			{"message", "`password` is required"}
		};
	}
	auto username = params["username"].as_string();
	bool is_superuser = atof(params["is_superuser"].as_string().c_str() );
	bserv::db_transaction tx{ conn };
	auto opt_user = get_user(tx, username);
	if (opt_user.has_value()) {
		return {
			{"success", false},
			{"message", "`username` existed"}
		};
	}
	auto password = params["password"].as_string();
	bserv::db_result r = tx.exec(
		"insert into ? "
		"(?, password, is_superuser, "
		"first_name, last_name, email, is_active) values "
		"(?, ?, ?, ?, ?, ?, ?)", bserv::db_name("auth_user"),
		bserv::db_name("username"),
		username,
		bserv::utils::security::encode_password(
			password.c_str()), is_superuser,
		get_or_empty(params, "first_name"),
		get_or_empty(params, "last_name"),
		get_or_empty(params, "email"), true);
	lginfo << r.query();
	tx.commit(); // you must manually commit changes
	return {
		{"success", true},
		{"message", "user registered"}
	};
}

boost::json::object add_canteen_register(
	bserv::request_type& request,
	// the json object is obtained from the request body,
	// as well as the url parameters
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn) {
	//if (request.method() != boost::beast::http::verb::post) {
	//	throw bserv::url_not_found_exception{};
	//}
	//if (params.count("username") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` is required"}
	//	};
	//}
	//if (params.count("password") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`password` is required"}
	//	};
	//}
	int C_ = atof(params["C_"].as_string().c_str());
	auto Cname = params["Cname"].as_string();
	auto Cpicture = params["Cpicture"].as_string();
	bserv::db_transaction tx{ conn };
	//auto opt_user = get_user(tx, username);
	//if (opt_user.has_value()) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` existed"}
	//	};
	//}
	bserv::db_result r = tx.exec(
		"insert into ? "
		"(C_, Cname, Cpicture) "
		"values "
		"(?, ?, ?)", bserv::db_name("canteen"),
		C_, Cname, Cpicture);
	lginfo << r.query();
	tx.commit(); // you must manually commit changes
	return {
		{"success", true},
		{"message", "user registered"}
	};
}

boost::json::object add_window_register(
	bserv::request_type& request,
	// the json object is obtained from the request body,
	// as well as the url parameters
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn) {
	//if (request.method() != boost::beast::http::verb::post) {
	//	throw bserv::url_not_found_exception{};
	//}
	//if (params.count("username") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` is required"}
	//	};
	//}
	//if (params.count("password") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`password` is required"}
	//	};
	//}
	auto Wname = params["Wname"].as_string();
	auto Wlocation = params["Wlocation"].as_string();
	auto Cname = params["Cname"].as_string();
	bserv::db_transaction tx{ conn };
	//auto opt_user = get_user(tx, username);
	//if (opt_user.has_value()) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` existed"}
	//	};
	//}
	bserv::db_result r = tx.exec("insert into ? "
		"(Wname, Wlocation, C_)  "
		"values "
		"(?, ?, "
		"(select C_ from canteen where Cname = ?) ); ",
		bserv::db_name("win"), Wname, Wlocation, Cname);
	lginfo << r.query();
	tx.commit(); // you must manually commit changes
	return {
		{"success", true},
		{"message", "user registered"}
	};
}

boost::json::object add_dish_register(
	bserv::request_type& request,
	// the json object is obtained from the request body,
	// as well as the url parameters
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn) {
	//if (request.method() != boost::beast::http::verb::post) {
	//	throw bserv::url_not_found_exception{};
	//}
	//if (params.count("username") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` is required"}
	//	};
	//}
	//if (params.count("password") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`password` is required"}
	//	};
	//}
	auto Dname = params["Dname"].as_string();
	double Dprice = atof(params["Dprice"].as_string().c_str() );
	//double is_sell = atof(params["is_sell"].as_string().c_str() );
	auto Dpicture = params["Dpicture"].as_string();
	auto Wname = params["Wname"].as_string();
	auto Cname = params["Cname"].as_string();
	bserv::db_transaction tx{ conn };
	//auto opt_user = get_user(tx, username);
	//if (opt_user.has_value()) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` existed"}
	//	};
	//}
	//auto password = params["password"].as_string();
	bserv::db_result r = tx.exec("INSERT into dish "
		"(Dname, Dprice, is_sell, Dpicture, W_) values "
		"(?, ?, TRUE, ?, "
		"(select win.W_ from win, canteen where win.C_ = canteen.C_ and win.Wname = ? and canteen.Cname = ? )  "
		");", Dname, Dprice, Dpicture, Wname, Cname );
	lginfo << r.query();
	tx.commit(); // you must manually commit changes
	return {
		{"success", true},
		{"message", "user registered"}
	};
}

boost::json::object add_tag_register(
	bserv::request_type& request,
	// the json object is obtained from the request body,
	// as well as the url parameters
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn) {
	//if (request.method() != boost::beast::http::verb::post) {
	//	throw bserv::url_not_found_exception{};
	//}
	//if (params.count("username") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` is required"}
	//	};
	//}
	//if (params.count("password") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`password` is required"}
	//	};
	//}
	auto Tname = params["Tname"].as_string();
	bserv::db_transaction tx{ conn };
	//auto opt_user = get_user(tx, username);
	//if (opt_user.has_value()) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` existed"}
	//	};
	//}
	//auto password = params["password"].as_string();
	std::cout << "1" << std::endl;
	bserv::db_result r = tx.exec("INSERT into tag "
		"(Tname, Tsupport) values "
		"(?, 0);", Tname);
	std::cout << "2" << std::endl;
	lginfo << r.query();
	tx.commit(); // you must manually commit changes
	std::cout << "3" << std::endl;
	return {
		{"success", true},
		{"message", "user registered"}
	};
}

boost::json::object add_dish_tag_register(
	bserv::request_type& request,
	// the json object is obtained from the request body,
	// as well as the url parameters
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn) {
	//if (request.method() != boost::beast::http::verb::post) {
	//	throw bserv::url_not_found_exception{};
	//}
	//if (params.count("username") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` is required"}
	//	};
	//}
	//if (params.count("password") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`password` is required"}
	//	};
	//}
	auto Tname = params["Tname"].as_string();
	int D_ = atof(params["D_"].as_string().c_str());
	bserv::db_transaction tx{ conn };
	//auto opt_user = get_user(tx, username);
	//if (opt_user.has_value()) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` existed"}
	//	};
	//}
	//auto password = params["password"].as_string();
	bserv::db_result r = tx.exec("INSERT into tag_belong "
		"(T_, D_) values "
		"((select T_ from tag where tag.Tname = ?), ?);", Tname, D_);
	lginfo << r.query();
	tx.commit(); // you must manually commit changes
	return {
		{"success", true},
		{"message", "user registered"}
	};
}

boost::json::object delete_canteen_from_database(
	bserv::request_type& request,
	// the json object is obtained from the request body,
	// as well as the url parameters
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn) {
	//if (request.method() != boost::beast::http::verb::post) {
	//	throw bserv::url_not_found_exception{};
	//}
	//if (params.count("username") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` is required"}
	//	};
	//}
	//if (params.count("password") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`password` is required"}
	//	};
	//}
	int C_ = atof(params["C_"].as_string().c_str() );
	bserv::db_transaction tx{ conn };
	bserv::db_result r = tx.exec("delete from canteen where C_ = ?", C_);
	lginfo << r.query();
	tx.commit(); // you must manually commit changes
	return {
		{"success", true},
		{"message", "user registered"}
	};
}

boost::json::object delete_window_from_database(
	bserv::request_type& request,
	// the json object is obtained from the request body,
	// as well as the url parameters
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn) {
	//if (request.method() != boost::beast::http::verb::post) {
	//	throw bserv::url_not_found_exception{};
	//}
	//if (params.count("username") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` is required"}
	//	};
	//}
	//if (params.count("password") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`password` is required"}
	//	};
	//}
	int W_ = atof(params["W_"].as_string().c_str());
	bserv::db_transaction tx{ conn };
	bserv::db_result r = tx.exec("delete from win where W_ = ?", W_);
	lginfo << r.query();
	tx.commit(); // you must manually commit changes
	return {
		{"success", true},
		{"message", "user registered"}
	};
}

boost::json::object delete_dish_from_database(
	bserv::request_type& request,
	// the json object is obtained from the request body,
	// as well as the url parameters
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn) {
	//if (request.method() != boost::beast::http::verb::post) {
	//	throw bserv::url_not_found_exception{};
	//}
	//if (params.count("username") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` is required"}
	//	};
	//}
	//if (params.count("password") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`password` is required"}
	//	};
	//}
	int D_ = atof(params["D_"].as_string().c_str());
	bserv::db_transaction tx{ conn };
	bserv::db_result r = tx.exec("delete from dish where D_ = ?", D_);
	lginfo << r.query();
	tx.commit(); // you must manually commit changes
	return {
		{"success", true},
		{"message", "user registered"}
	};
}

boost::json::object delete_tag_from_database(
	bserv::request_type& request,
	// the json object is obtained from the request body,
	// as well as the url parameters
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn) {
	//if (request.method() != boost::beast::http::verb::post) {
	//	throw bserv::url_not_found_exception{};
	//}
	//if (params.count("username") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` is required"}
	//	};
	//}
	//if (params.count("password") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`password` is required"}
	//	};
	//}
	int T_ = atof(params["T_"].as_string().c_str());
	bserv::db_transaction tx{ conn };
	bserv::db_result s = tx.exec("delete from tag_belong where T_ = ?", T_);
	bserv::db_result r = tx.exec("delete from tag where T_ = ?", T_);
	lginfo << r.query();
	tx.commit(); // you must manually commit changes
	return {
		{"success", true},
		{"message", "user registered"}
	};
}

boost::json::object delete_dish_tag_from_database(
	bserv::request_type& request,
	// the json object is obtained from the request body,
	// as well as the url parameters
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn) {
	//if (request.method() != boost::beast::http::verb::post) {
	//	throw bserv::url_not_found_exception{};
	//}
	//if (params.count("username") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` is required"}
	//	};
	//}
	//if (params.count("password") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`password` is required"}
	//	};
	//}
	int T_ = atof(params["T_"].as_string().c_str());
	int D_ = atof(params["D_"].as_string().c_str());
	bserv::db_transaction tx{ conn };
	bserv::db_result r = tx.exec("delete from tag_belong where T_ = ? and D_ = ?", T_, D_);
	lginfo << r.query();
	tx.commit(); // you must manually commit changes
	return {
		{"success", true},
		{"message", "user registered"}
	};
}

boost::json::object delete_remark_from_database(
	bserv::request_type& request,
	// the json object is obtained from the request body,
	// as well as the url parameters
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn) {
	//if (request.method() != boost::beast::http::verb::post) {
	//	throw bserv::url_not_found_exception{};
	//}
	//if (params.count("username") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` is required"}
	//	};
	//}
	//if (params.count("password") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`password` is required"}
	//	};
	//}
	int R_ = atof(params["R_"].as_string().c_str());
	bserv::db_transaction tx{ conn };
	bserv::db_result r = tx.exec("delete from remark where R_ = ?", R_);
	lginfo << r.query();
	tx.commit(); // you must manually commit changes
	return {
		{"success", true},
		{"message", "user registered"}
	};
}

boost::json::object delete_user_from_database(
	bserv::request_type& request,
	// the json object is obtained from the request body,
	// as well as the url parameters
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn) {
	if (request.method() != boost::beast::http::verb::post) {
		throw bserv::url_not_found_exception{};
	}
	if (params.count("username") == 0) {
		return {
			{"success", false},
			{"message", "`username` is required"}
		};
	}
	if (params.count("password") == 0) {
		return {
			{"success", false},
			{"message", "`password` is required"}
		};
	}
	auto username = params["username"].as_string();
	bserv::db_transaction tx{ conn };
	auto opt_user = get_user(tx, username);
	if (opt_user.has_value()) {
		return {
			{"success", false},
			{"message", "`username` existed"}
		};
	}
	auto password = params["password"].as_string();
	bserv::db_result r = tx.exec(
		"insert into ? "
		"(?, password, is_superuser, "
		"first_name, last_name, email, is_active) values "
		"(?, ?, ?, ?, ?, ?, ?)", bserv::db_name("auth_user"),
		bserv::db_name("username"),
		username,
		bserv::utils::security::encode_password(
			password.c_str()), false,
		get_or_empty(params, "first_name"),
		get_or_empty(params, "last_name"),
		get_or_empty(params, "email"), true);
	lginfo << r.query();
	tx.commit(); // you must manually commit changes
	return {
		{"success", true},
		{"message", "user registered"}
	};
}

boost::json::object update_canteen_from_database(
	bserv::request_type& request,
	// the json object is obtained from the request body,
	// as well as the url parameters
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn) {
	//if (request.method() != boost::beast::http::verb::post) {
	//	throw bserv::url_not_found_exception{};
	//}
	//if (params.count("username") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` is required"}
	//	};
	//}
	//if (params.count("password") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`password` is required"}
	//	};
	//}
	int C_ = atof(params["C_"].as_string().c_str());
	auto Cname = params["Cname"].as_string();
	auto Cpicture = params["Cpicture"].as_string();
	bserv::db_transaction tx{ conn };
	//auto opt_user = get_user(tx, username);
	//if (opt_user.has_value()) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` existed"}
	//	};
	//}
	bserv::db_result r = tx.exec("update canteen set Cname = ?, Cpicture = ?  where C_=?;", Cname, Cpicture, C_);
	lginfo << r.query();
	tx.commit(); // you must manually commit changes
	return {
		{"success", true},
		{"message", "user registered"}
	};
}

boost::json::object update_window_from_database(
	bserv::request_type& request,
	// the json object is obtained from the request body,
	// as well as the url parameters
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn) {
	//if (request.method() != boost::beast::http::verb::post) {
	//	throw bserv::url_not_found_exception{};
	//}
	//if (params.count("username") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` is required"}
	//	};
	//}
	//if (params.count("password") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`password` is required"}
	//	};
	//}
	auto Wname = params["Wname"].as_string();
	auto Wlocation = params["Wlocation"].as_string();
	auto Cname = params["Cname"].as_string();
	int W_ = atof(params["W_"].as_string().c_str());
	bserv::db_transaction tx{ conn };
	//auto opt_user = get_user(tx, username);
	//if (opt_user.has_value()) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` existed"}
	//	};
	//}
	bserv::db_result r = tx.exec("update win set Wname = ?, Wlocation = ?, C_ = (select C_ from canteen where Cname = ?) where W_ = ?", 
								Wname, Wlocation, Cname, W_);
	lginfo << r.query();
	tx.commit(); // you must manually commit changes
	return {
		{"success", true},
		{"message", "user registered"}
	};
}

boost::json::object update_dish_from_database(
	bserv::request_type& request,
	// the json object is obtained from the request body,
	// as well as the url parameters
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn) {
	//if (request.method() != boost::beast::http::verb::post) {
	//	throw bserv::url_not_found_exception{};
	//}
	//if (params.count("username") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` is required"}
	//	};
	//}
	//if (params.count("password") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`password` is required"}
	//	};
	//}
	auto Dname = params["Dname"].as_string();
	double Dprice = atof(params["Dprice"].as_string().c_str());
	std::string is_sell_string = params["is_sell"].as_string().c_str();
	bool is_sell;
	if (is_sell_string == "true")
		is_sell = 1;
	else
		is_sell = 0;
	std::cout << "string =" << params["is_sell"].as_string().c_str() << std::endl;
	std::cout << "bool =" << atof(params["is_sell"].as_string().c_str()) << std::endl;
	auto Dpicture = params["Dpicture"].as_string();
	auto Wname = params["Wname"].as_string();
	auto Cname = params["Cname"].as_string();
	int D_ = atof(params["D_"].as_string().c_str());
	bserv::db_transaction tx{ conn };
	//auto opt_user = get_user(tx, username);
	//if (opt_user.has_value()) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` existed"}
	//	};
	//}
	//auto password = params["password"].as_string();
	bserv::db_result r = tx.exec("update dish set Dname = ?, Dprice = ?, is_sell = ?, Dpicture = ?, "
						"W_= (select win.W_ from win, canteen where win.C_ = canteen.C_ and Cname = ? and Wname = ?) where D_ = ? ",
						Dname, Dprice, is_sell, Dpicture, Cname, Wname, D_);
	lginfo << r.query();
	tx.commit(); // you must manually commit changes
	return {
		{"success", true},
		{"message", "user registered"}
	};
}

boost::json::object update_tag_from_database(
	bserv::request_type& request,
	// the json object is obtained from the request body,
	// as well as the url parameters
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn) {
	//if (request.method() != boost::beast::http::verb::post) {
	//	throw bserv::url_not_found_exception{};
	//}
	//if (params.count("username") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` is required"}
	//	};
	//}
	//if (params.count("password") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`password` is required"}
	//	};
	//}
	auto Tname = params["Tname"].as_string();
	int T_ = atof(params["T_"].as_string().c_str());
	bserv::db_transaction tx{ conn };
	//auto opt_user = get_user(tx, username);
	//if (opt_user.has_value()) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` existed"}
	//	};
	//}
	//auto password = params["password"].as_string();
	bserv::db_result r = tx.exec("update tag set Tname = ? where T_ = ?", Tname, T_);
	lginfo << r.query();
	tx.commit(); // you must manually commit changes
	return {
		{"success", true},
		{"message", "user registered"}
	};
}

boost::json::object user_login(
	bserv::request_type& request,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr) {
	if (request.method() != boost::beast::http::verb::post) {
		throw bserv::url_not_found_exception{};
	}
	if (params.count("username") == 0) {
		return {
			{"success", false},
			{"message", "`username` is required"}
		};
	}
	if (params.count("password") == 0) {
		return {
			{"success", false},
			{"message", "`password` is required"}
		};
	}
	auto username = params["username"].as_string();
	bserv::db_transaction tx{ conn };
	auto opt_user = get_user(tx, username);
	if (!opt_user.has_value()) {
		return {
			{"success", false},
			{"message", "invalid username/password"}
		};
	}
	auto& user = opt_user.value();
	if (!user["is_active"].as_bool()) {
		return {
			{"success", false},
			{"message", "invalid username/password"}
		};
	}
	auto password = params["password"].as_string();
	auto encoded_password = user["password"].as_string();
	if (!bserv::utils::security::check_password(
		password.c_str(), encoded_password.c_str())) {
		return {
			{"success", false},
			{"message", "invalid username/password"}
		};
	}
	bserv::session_type& session = *session_ptr;
	session["user"] = user;
	auto superuser = user;
	bool is_superuser = user["is_superuser"].as_bool();
	std::cout << "是不是超管" << is_superuser << std::endl;
	if (is_superuser == 1)
	{
		std::cout << "放好了" << std::endl;
		session["superuser"] = superuser;
		std::cout << "是不是超管2" << session["superuser"].as_object()["is_superuser"].as_bool() << std::endl;
	}

	//if (is_superuser == 1)
	//{
	//	//std::cout << "放好了" << std::endl;
	//	return {
	//	{"success", true},
	//	{"message", "login successfully"},
	//	{"is_superuser", true}
	//	};
	//}

	return {
		{"success", true},
		{"message", "login successfully"}
	};
}

boost::json::object find_user(
	std::shared_ptr<bserv::db_connection> conn,
	const std::string& username) {
	bserv::db_transaction tx{ conn };
	auto user = get_user(tx, username.c_str());
	if (!user.has_value()) {
		return {
			{"success", false},
			{"message", "requested user does not exist"}
		};
	}
	user.value().erase("id");
	user.value().erase("password");
	return {
		{"success", true},
		{"user", user.value()}
	};
}

boost::json::object user_logout(
	std::shared_ptr<bserv::session_type> session_ptr) {
	bserv::session_type& session = *session_ptr;
	if (session.count("user")) {
		session.erase("user");
	}
	if (session.count("superuser")) {
		session.erase("superuser");
	}
	return {
		{"success", true},
		{"message", "logout successfully"}
	};
}

boost::json::object send_request(
	std::shared_ptr<bserv::session_type> session,
	std::shared_ptr<bserv::http_client> client_ptr,
	boost::json::object&& params) {
	// post for response:
	// auto res = client_ptr->post(
	//     "localhost", "8080", "/echo", {{"msg", "request"}}
	// );
	// return {{"response", boost::json::parse(res.body())}};
	// -------------------------------------------------------
	// - if it takes longer than 30 seconds (by default) to
	// - get the response, this will raise a read timeout
	// -------------------------------------------------------
	// post for json response (json value, rather than json
	// object, is returned):
	auto obj = client_ptr->post_for_value(
		"localhost", "8080", "/echo", { {"request", params} }
	);
	if (session->count("cnt") == 0) {
		(*session)["cnt"] = 0;
	}
	(*session)["cnt"] = (*session)["cnt"].as_int64() + 1;
	return { {"response", obj}, {"cnt", (*session)["cnt"]} };
}

boost::json::object echo(
	boost::json::object&& params) {
	return { {"echo", params} };
}

// websocket
std::nullopt_t ws_echo(
	std::shared_ptr<bserv::session_type> session,
	std::shared_ptr<bserv::websocket_server> ws_server) {
	ws_server->write_json((*session)["cnt"]);
	while (true) {
		try {
			std::string data = ws_server->read();
			ws_server->write(data);
		}
		catch (bserv::websocket_closed&) {
			break;
		}
	}
	return std::nullopt;
}


std::nullopt_t serve_static_files(
	bserv::response_type& response,
	const std::string& path) {
	return serve(response, path);
}

//主页——index
std::nullopt_t index(
	const std::string& template_path,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response,
	boost::json::object& context) {
	bserv::session_type& session = *session_ptr;
	if (session.contains("user")) {
		context["user"] = session["user"];
	}
	if (session.contains("superuser")) {
		context["superuser"] = session["superuser"];
	}
	return render(response, template_path, context);
}

std::nullopt_t index_page(
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response) {
	boost::json::object context;

	lgdebug << "view canteen: " << std::endl;
	bserv::db_transaction tx{ conn };
	bserv::db_result db_res = tx.exec("select count(*) from canteen;");
	lginfo << db_res.query();
	std::size_t total_canteens = (*db_res.begin())[0].as<std::size_t>();
	lgdebug << "total canteens: " << total_canteens << std::endl;
	db_res = tx.exec("select * from canteen;");
	lginfo << db_res.query();
	auto canteens = orm_canteen.convert_to_vector(db_res);
	boost::json::array json_canteens;
	for (auto& canteen : canteens) {
		json_canteens.push_back(canteen);
	}
	context["canteens"] = json_canteens;
	return index("index.html", session_ptr, response, context);
}

std::nullopt_t form_login(
	bserv::request_type& request,
	bserv::response_type& response,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr) {
	lgdebug << params << std::endl;
	auto context = user_login(request, std::move(params), conn, session_ptr);

	lgdebug << "view canteen: " << std::endl;
	bserv::db_transaction tx{ conn };
	bserv::db_result db_res = tx.exec("select count(*) from canteen;");
	lginfo << db_res.query();
	std::size_t total_canteens = (*db_res.begin())[0].as<std::size_t>();
	lgdebug << "total canteens: " << total_canteens << std::endl;
	db_res = tx.exec("select * from canteen;");
	lginfo << db_res.query();
	auto canteens = orm_canteen.convert_to_vector(db_res);
	boost::json::array json_canteens;
	for (auto& canteen : canteens) {
		json_canteens.push_back(canteen);
	}
	context["canteens"] = json_canteens;
	lginfo << "login: " << context << std::endl;
	return index("index.html", session_ptr, response, context);
}

std::nullopt_t form_logout(
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response) {
	auto context = user_logout(session_ptr);
	lgdebug << "view canteen: " << std::endl;
	bserv::db_transaction tx{ conn };
	bserv::db_result db_res = tx.exec("select count(*) from canteen;");
	lginfo << db_res.query();
	std::size_t total_canteens = (*db_res.begin())[0].as<std::size_t>();
	lgdebug << "total canteens: " << total_canteens << std::endl;
	db_res = tx.exec("select * from canteen;");
	lginfo << db_res.query();
	auto canteens = orm_canteen.convert_to_vector(db_res);
	boost::json::array json_canteens;
	for (auto& canteen : canteens) {
		json_canteens.push_back(canteen);
	}
	context["canteens"] = json_canteens;

	lginfo << "logout: " << context << std::endl;
	return index("index.html", session_ptr, response, context);
}

std::nullopt_t redirect_to_users(
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response,
	int page_id,
	boost::json::object&& context) {
	lgdebug << "view users: " << page_id << std::endl;
	bserv::db_transaction tx{ conn };
	bserv::db_result db_res = tx.exec("select count(*) from auth_user;");
	lginfo << db_res.query();
	std::size_t total_users = (*db_res.begin())[0].as<std::size_t>();
	lgdebug << "total users: " << total_users << std::endl;
	int total_pages = (int)total_users / 10;
	if (total_users % 10 != 0) ++total_pages;
	lgdebug << "total pages: " << total_pages << std::endl;
	db_res = tx.exec("select * from auth_user limit 10 offset ?;", (page_id - 1) * 10);
	lginfo << db_res.query();
	auto users = orm_user.convert_to_vector(db_res);
	boost::json::array json_users;
	for (auto& user : users) {
		json_users.push_back(user);
	}
	boost::json::object pagination;
	if (total_pages != 0) {
		pagination["total"] = total_pages;
		if (page_id > 1) {
			pagination["previous"] = page_id - 1;
		}
		if (page_id < total_pages) {
			pagination["next"] = page_id + 1;
		}
		int lower = page_id - 3;
		int upper = page_id + 3;
		if (page_id - 3 > 2) {
			pagination["left_ellipsis"] = true;
		}
		else {
			lower = 1;
		}
		if (page_id + 3 < total_pages - 1) {
			pagination["right_ellipsis"] = true;
		}
		else {
			upper = total_pages;
		}
		pagination["current"] = page_id;
		boost::json::array pages_left;
		for (int i = lower; i < page_id; ++i) {
			pages_left.push_back(i);
		}
		pagination["pages_left"] = pages_left;
		boost::json::array pages_right;
		for (int i = page_id + 1; i <= upper; ++i) {
			pages_right.push_back(i);
		}
		pagination["pages_right"] = pages_right;
		context["pagination"] = pagination;
	}
	context["users"] = json_users;
	return index("users.html", session_ptr, response, context);
}

std::nullopt_t redirect_to_users_login(
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response,
	int page_id,
	boost::json::object&& context) {
	lgdebug << "view users: " << page_id << std::endl;
	bserv::db_transaction tx{ conn };
	bserv::db_result db_res = tx.exec("select count(*) from auth_user;");
	lginfo << db_res.query();
	std::size_t total_users = (*db_res.begin())[0].as<std::size_t>();
	lgdebug << "total users: " << total_users << std::endl;
	int total_pages = (int)total_users / 10;
	if (total_users % 10 != 0) ++total_pages;
	lgdebug << "total pages: " << total_pages << std::endl;
	db_res = tx.exec("select * from auth_user limit 10 offset ?;", (page_id - 1) * 10);
	lginfo << db_res.query();
	auto users = orm_user.convert_to_vector(db_res);
	boost::json::array json_users;
	for (auto& user : users) {
		json_users.push_back(user);
	}
	boost::json::object pagination;
	if (total_pages != 0) {
		pagination["total"] = total_pages;
		if (page_id > 1) {
			pagination["previous"] = page_id - 1;
		}
		if (page_id < total_pages) {
			pagination["next"] = page_id + 1;
		}
		int lower = page_id - 3;
		int upper = page_id + 3;
		if (page_id - 3 > 2) {
			pagination["left_ellipsis"] = true;
		}
		else {
			lower = 1;
		}
		if (page_id + 3 < total_pages - 1) {
			pagination["right_ellipsis"] = true;
		}
		else {
			upper = total_pages;
		}
		pagination["current"] = page_id;
		boost::json::array pages_left;
		for (int i = lower; i < page_id; ++i) {
			pages_left.push_back(i);
		}
		pagination["pages_left"] = pages_left;
		boost::json::array pages_right;
		for (int i = page_id + 1; i <= upper; ++i) {
			pages_right.push_back(i);
		}
		pagination["pages_right"] = pages_right;
		context["pagination"] = pagination;
	}
	context["users"] = json_users;

	lgdebug << "view canteen: " << std::endl;
	db_res = tx.exec("select count(*) from canteen;");
	lginfo << db_res.query();
	std::size_t total_canteens = (*db_res.begin())[0].as<std::size_t>();
	lgdebug << "total canteens: " << total_canteens << std::endl;
	db_res = tx.exec("select * from canteen;");
	lginfo << db_res.query();
	auto canteens = orm_canteen.convert_to_vector(db_res);
	boost::json::array json_canteens;
	for (auto& canteen : canteens) {
		json_canteens.push_back(canteen);
	}
	context["canteens"] = json_canteens;
	return index("index.html", session_ptr, response, context);
}

std::nullopt_t redirect_to_canteen(
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response,
	int page_id,
	boost::json::object&& context) {
	lgdebug << "view canteens: " << page_id << std::endl;
	bserv::db_transaction tx{ conn };
	bserv::db_result db_res = tx.exec("select count(*) from canteen;");
	lginfo << db_res.query();
	std::size_t total_canteens = (*db_res.begin())[0].as<std::size_t>();
	lgdebug << "total canteens: " << total_canteens << std::endl;
	int total_pages = (int)total_canteens / 10;
	if (total_canteens % 10 != 0) ++total_pages;
	lgdebug << "total pages: " << total_pages << std::endl;
	db_res = tx.exec("select * from canteen limit 10 offset ?;", (page_id - 1) * 10);
	lginfo << db_res.query();
	auto canteens = orm_canteen.convert_to_vector(db_res);
	boost::json::array json_canteens;
	for (auto& canteen : canteens) {
		json_canteens.push_back(canteen);
	}
	boost::json::object pagination;
	if (total_pages != 0) {
		pagination["total"] = total_pages;
		if (page_id > 1) {
			pagination["previous"] = page_id - 1;
		}
		if (page_id < total_pages) {
			pagination["next"] = page_id + 1;
		}
		int lower = page_id - 3;
		int upper = page_id + 3;
		if (page_id - 3 > 2) {
			pagination["left_ellipsis"] = true;
		}
		else {
			lower = 1;
		}
		if (page_id + 3 < total_pages - 1) {
			pagination["right_ellipsis"] = true;
		}
		else {
			upper = total_pages;
		}
		pagination["current"] = page_id;
		boost::json::array pages_left;
		for (int i = lower; i < page_id; ++i) {
			pages_left.push_back(i);
		}
		pagination["pages_left"] = pages_left;
		boost::json::array pages_right;
		for (int i = page_id + 1; i <= upper; ++i) {
			pages_right.push_back(i);
		}
		pagination["pages_right"] = pages_right;
		context["pagination"] = pagination;
	}
	context["canteens"] = json_canteens;
	return index("canteen_management.html", session_ptr, response, context);
}

std::nullopt_t redirect_to_window(
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response,
	int page_id,
	boost::json::object&& context) {
	lgdebug << "view windows: " << page_id << std::endl;
	bserv::db_transaction tx{ conn };
	bserv::db_result db_res = tx.exec("select count(*) from win, canteen where win.C_=canteen.C_;");
	lginfo << db_res.query();
	std::size_t total_windows = (*db_res.begin())[0].as<std::size_t>();
	lgdebug << "total windows: " << total_windows << std::endl;
	int total_pages = (int)total_windows / 10;
	if (total_windows % 10 != 0) ++total_pages;
	lgdebug << "total pages: " << total_pages << std::endl;
	db_res = tx.exec("select W_, Wname, Wlocation, win.C_, Cname, Cpicture from win, canteen where win.C_=canteen.C_ limit 10 offset ?;", (page_id - 1) * 10);
	lginfo << db_res.query();
	auto windows = orm_window_management.convert_to_vector(db_res);
	boost::json::array json_windows;
	for (auto& window : windows) {
		json_windows.push_back(window);
	}
	boost::json::object pagination;
	if (total_pages != 0) {
		pagination["total"] = total_pages;
		if (page_id > 1) {
			pagination["previous"] = page_id - 1;
		}
		if (page_id < total_pages) {
			pagination["next"] = page_id + 1;
		}
		int lower = page_id - 3;
		int upper = page_id + 3;
		if (page_id - 3 > 2) {
			pagination["left_ellipsis"] = true;
		}
		else {
			lower = 1;
		}
		if (page_id + 3 < total_pages - 1) {
			pagination["right_ellipsis"] = true;
		}
		else {
			upper = total_pages;
		}
		pagination["current"] = page_id;
		boost::json::array pages_left;
		for (int i = lower; i < page_id; ++i) {
			pages_left.push_back(i);
		}
		pagination["pages_left"] = pages_left;
		boost::json::array pages_right;
		for (int i = page_id + 1; i <= upper; ++i) {
			pages_right.push_back(i);
		}
		pagination["pages_right"] = pages_right;
		context["pagination"] = pagination;
	}
	context["windows"] = json_windows;
	return index("window_management.html", session_ptr, response, context);
}

std::nullopt_t redirect_to_dish(
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response,
	int page_id,
	boost::json::object&& context) {
	lgdebug << "view dishes: " << page_id << std::endl;
	bserv::db_transaction tx{ conn };
	bserv::db_result db_res = tx.exec("select count(*) from dish, win, canteen where dish.W_=win.W_ and win.C_=canteen.C_ ;");
	lginfo << db_res.query();
	std::size_t total_dishes = (*db_res.begin())[0].as<std::size_t>();
	lgdebug << "total dishes: " << total_dishes << std::endl;
	int total_pages = (int)total_dishes / 10;
	if (total_dishes % 10 != 0) ++total_pages;
	lgdebug << "total pages: " << total_pages << std::endl;
	db_res = tx.exec("select D_, Dname,  Dprice, is_sell, Dpicture, win.W_, Wname, Wlocation, canteen.C_, Cname, Cpicture from dish, win, canteen where dish.W_=win.W_ and win.C_=canteen.C_ limit 10 offset ?;", (page_id - 1) * 10);
	lginfo << db_res.query();
	auto dishes = orm_dish_management.convert_to_vector(db_res);
	boost::json::array json_dishes;
	for (auto& dish : dishes) {
		json_dishes.push_back(dish);
	}
	boost::json::object pagination;
	if (total_pages != 0) {
		pagination["total"] = total_pages;
		if (page_id > 1) {
			pagination["previous"] = page_id - 1;
		}
		if (page_id < total_pages) {
			pagination["next"] = page_id + 1;
		}
		int lower = page_id - 3;
		int upper = page_id + 3;
		if (page_id - 3 > 2) {
			pagination["left_ellipsis"] = true;
		}
		else {
			lower = 1;
		}
		if (page_id + 3 < total_pages - 1) {
			pagination["right_ellipsis"] = true;
		}
		else {
			upper = total_pages;
		}
		pagination["current"] = page_id;
		boost::json::array pages_left;
		for (int i = lower; i < page_id; ++i) {
			pages_left.push_back(i);
		}
		pagination["pages_left"] = pages_left;
		boost::json::array pages_right;
		for (int i = page_id + 1; i <= upper; ++i) {
			pages_right.push_back(i);
		}
		pagination["pages_right"] = pages_right;
		context["pagination"] = pagination;
	}
	context["dishes"] = json_dishes;
	return index("dish_management.html", session_ptr, response, context);
}

std::nullopt_t redirect_to_dish_search(
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response,
	int page_id,
	boost::json::object&& context,
	std::string Dname_search) {
	lgdebug << "view dishes: " << page_id << std::endl;
	bserv::db_transaction tx{ conn };
	bserv::db_result db_res = tx.exec("select count(*) from dish, win, canteen where dish.W_=win.W_ and win.C_=canteen.C_ and Dname like ?;", Dname_search + "%");
	lginfo << db_res.query();
	std::size_t total_dishes = (*db_res.begin())[0].as<std::size_t>();
	lgdebug << "total dishes: " << total_dishes << std::endl;
	int total_pages = (int)total_dishes / 10;
	if (total_dishes % 10 != 0) ++total_pages;
	lgdebug << "total pages: " << total_pages << std::endl;
	db_res = tx.exec("select D_, Dname,  Dprice, is_sell, Dpicture, win.W_, Wname, Wlocation, canteen.C_, Cname, Cpicture from dish, win, canteen where dish.W_=win.W_ and win.C_=canteen.C_ and dish.Dname like ? limit 10 offset ?;", Dname_search + "%", (page_id - 1) * 10);
	lginfo << db_res.query();
	auto dishes = orm_dish_management.convert_to_vector(db_res);
	boost::json::array json_dishes;
	for (auto& dish : dishes) {
		json_dishes.push_back(dish);
	}
	boost::json::object pagination;
	if (total_pages != 0) {
		pagination["total"] = total_pages;
		if (page_id > 1) {
			pagination["previous"] = page_id - 1;
		}
		if (page_id < total_pages) {
			pagination["next"] = page_id + 1;
		}
		int lower = page_id - 3;
		int upper = page_id + 3;
		if (page_id - 3 > 2) {
			pagination["left_ellipsis"] = true;
		}
		else {
			lower = 1;
		}
		if (page_id + 3 < total_pages - 1) {
			pagination["right_ellipsis"] = true;
		}
		else {
			upper = total_pages;
		}
		pagination["current"] = page_id;
		boost::json::array pages_left;
		for (int i = lower; i < page_id; ++i) {
			pages_left.push_back(i);
		}
		pagination["pages_left"] = pages_left;
		boost::json::array pages_right;
		for (int i = page_id + 1; i <= upper; ++i) {
			pages_right.push_back(i);
		}
		pagination["pages_right"] = pages_right;
		context["pagination"] = pagination;
	}
	context["dishes"] = json_dishes;
	return index("dish_management.html", session_ptr, response, context);
}

std::nullopt_t redirect_to_tag(
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response,
	int page_id,
	boost::json::object&& context) {
	lgdebug << "view tags: " << page_id << std::endl;
	bserv::db_transaction tx{ conn };
	bserv::db_result db_res = tx.exec("select count(*) from tag ;");
	lginfo << db_res.query();
	std::size_t total_tags = (*db_res.begin())[0].as<std::size_t>();
	lgdebug << "total tags: " << total_tags << std::endl;
	int total_pages = (int)total_tags / 10;
	if (total_tags % 10 != 0) ++total_pages;
	lgdebug << "total pages: " << total_pages << std::endl;
	db_res = tx.exec("select T_, Tname from tag limit 10 offset ?;", (page_id - 1) * 10);
	lginfo << db_res.query();
	auto tags = orm_tag_management.convert_to_vector(db_res);
	boost::json::array json_tags;
	for (auto& tag : tags) {
		json_tags.push_back(tag);
	}
	boost::json::object pagination;
	if (total_pages != 0) {
		pagination["total"] = total_pages;
		if (page_id > 1) {
			pagination["previous"] = page_id - 1;
		}
		if (page_id < total_pages) {
			pagination["next"] = page_id + 1;
		}
		int lower = page_id - 3;
		int upper = page_id + 3;
		if (page_id - 3 > 2) {
			pagination["left_ellipsis"] = true;
		}
		else {
			lower = 1;
		}
		if (page_id + 3 < total_pages - 1) {
			pagination["right_ellipsis"] = true;
		}
		else {
			upper = total_pages;
		}
		pagination["current"] = page_id;
		boost::json::array pages_left;
		for (int i = lower; i < page_id; ++i) {
			pages_left.push_back(i);
		}
		pagination["pages_left"] = pages_left;
		boost::json::array pages_right;
		for (int i = page_id + 1; i <= upper; ++i) {
			pages_right.push_back(i);
		}
		pagination["pages_right"] = pages_right;
		context["pagination"] = pagination;
	}
	context["tags"] = json_tags;
	return index("tag_management.html", session_ptr, response, context);
}

std::nullopt_t redirect_to_dish_tag(
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response,
	int page_id,
	int dish_id,
	boost::json::object&& context) {
	lgdebug << "view dish_tags: " << page_id << std::endl;
	bserv::db_transaction tx{ conn };
	bserv::db_result db_res = tx.exec("select count(*) from tag, tag_belong where tag.T_ = tag_belong.T_ and tag_belong.D_ = ? ;", dish_id);
	lginfo << db_res.query();
	std::size_t total_dish_tags = (*db_res.begin())[0].as<std::size_t>();
	lgdebug << "total dish_tags: " << total_dish_tags << std::endl;
	int total_pages = (int)total_dish_tags / 10;
	if (total_dish_tags % 10 != 0) ++total_pages;
	lgdebug << "total pages: " << total_pages << std::endl;
	db_res = tx.exec("select tag.T_, tag.Tname from tag, tag_belong where tag.T_ = tag_belong.T_ and tag_belong.D_ = ? limit 10 offset ?;", dish_id, (page_id - 1) * 10);
	lginfo << db_res.query();
	auto dish_tags = orm_tag_management.convert_to_vector(db_res);
	boost::json::array json_dish_tags;
	for (auto& dish_tag : dish_tags) {
		json_dish_tags.push_back(dish_tag);
	}
	boost::json::object pagination;
	if (total_pages != 0) {
		pagination["total"] = total_pages;
		if (page_id > 1) {
			pagination["previous"] = page_id - 1;
		}
		if (page_id < total_pages) {
			pagination["next"] = page_id + 1;
		}
		int lower = page_id - 3;
		int upper = page_id + 3;
		if (page_id - 3 > 2) {
			pagination["left_ellipsis"] = true;
		}
		else {
			lower = 1;
		}
		if (page_id + 3 < total_pages - 1) {
			pagination["right_ellipsis"] = true;
		}
		else {
			upper = total_pages;
		}
		pagination["current"] = page_id;
		boost::json::array pages_left;
		for (int i = lower; i < page_id; ++i) {
			pages_left.push_back(i);
		}
		pagination["pages_left"] = pages_left;
		boost::json::array pages_right;
		for (int i = page_id + 1; i <= upper; ++i) {
			pages_right.push_back(i);
		}
		pagination["pages_right"] = pages_right;
		context["pagination"] = pagination;
	}
	context["tags"] = json_dish_tags;

	db_res = tx.exec("select * from dish where D_ = ? ;", dish_id);
	lginfo << db_res.query();
	auto dishes = orm_dish.convert_to_vector(db_res);
	boost::json::array json_dishes;
	for (auto& dish : dishes) {
		json_dishes.push_back(dish);
	}
	context["dishes"] = json_dishes;
	return index("dish_tag.html", session_ptr, response, context);
}

std::nullopt_t redirect_to_dish(
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response,
	boost::json::object&& context,
	int canteen_num,
	int table_num,
	int tag_num,
	int dish_num) {
	lgdebug << "view dish_content: " << std::endl;
	bserv::db_transaction tx{ conn };

	//搜索菜品信息
	bserv::db_result db_res = tx.exec("select count(*) from dish where dish.D_ = ?", dish_num);
	lginfo << db_res.query();
	std::size_t total_dishes = (*db_res.begin())[0].as<std::size_t>();
	lgdebug << "total dishes: " << total_dishes << std::endl;
	db_res = tx.exec("select * from dish where dish.D_ = ?", dish_num);
	lginfo << db_res.query();

	auto dishes = orm_dish.convert_to_vector(db_res);
	boost::json::array json_dishes;
	for (auto& dish : dishes) {
		json_dishes.push_back(dish);
	}
	context["dishes"] = json_dishes;

	//搜索评论信息
	db_res = tx.exec("select count(*) from remark, auth_user where remark.D_ = ? and remark.id = auth_user.id", dish_num);
	lginfo << db_res.query();
	std::size_t total_remarks = (*db_res.begin())[0].as<std::size_t>();
	lgdebug << "total remarks: " << total_remarks << std::endl;
	db_res = tx.exec("select R_, Rcontext, Rmark, auth_user.id, username, D_ from remark, auth_user where remark.D_ = ? and remark.id = auth_user.id", dish_num);
	lginfo << db_res.query();

	auto remarks = orm_remark.convert_to_vector(db_res);
	boost::json::array json_remarks;
	for (auto& remark : remarks) {
		json_remarks.push_back(remark);
	}
	context["remarks"] = json_remarks;


	//计算评分score
	db_res = tx.exec("select AVG(Rmark) from remark where remark.D_ = ?", dish_num);
	lginfo << db_res.query();
	//std::size_t total_score = (*db_res.begin())[0].as<std::size_t>();
	//lgdebug << "total score: " << total_score << std::endl;
	db_res = tx.exec("select floor(AVG(Rmark)) from remark where remark.D_ = ?", dish_num);
	lginfo << db_res.query();
	if (total_remarks != 0)
	{
		auto score = orm_score.convert_to_vector(db_res);
		std::cout << "到这里" << std::endl;
		boost::json::array json_score;
		for (auto& score_single : score) {
			json_score.push_back(score_single);
		}
		context["score"] = json_score;
	}
	else
	{
		std::vector<boost::json::object> score;
		std::cout << "到这里" << std::endl;
		boost::json::array json_score;
		for (auto& score_single : score) {
			json_score.push_back(score_single);
		}
		context["score"] = json_score;
	}
	
	//搜索菜品标签
	db_res = tx.exec("select count(*) from tag, tag_belong where tag_belong.D_ = ? and tag.T_ = tag_belong.T_", dish_num);
	lginfo << db_res.query();
	std::size_t total_tags = (*db_res.begin())[0].as<std::size_t>();
	lgdebug << "total tags: " << total_tags << std::endl;
	db_res = tx.exec("select * from tag, tag_belong where tag_belong.D_ = ? and tag.T_ = tag_belong.T_", dish_num);
	lginfo << db_res.query();

	if (total_tags != 0)
	{
		auto tags = orm_tag.convert_to_vector(db_res);
		boost::json::array json_tags;
		for (auto& tag : tags) {
			json_tags.push_back(tag);
		}
		context["tags"] = json_tags;
	}
	else
	{
		std::vector<boost::json::object>  tags;
		boost::json::array json_tags;
		for (auto& tag : tags) {
			json_tags.push_back(tag);
		}
		context["tags"] = json_tags;
	}

	return index("dishes_content.html", session_ptr, response, context);
}


std::nullopt_t redirect_to_canteen_index(
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response,
	boost::json::object&& context,
	int canteen_num,
	int table_num, 
	int tag_num,
	std::string dish_search) {
	lgdebug << "view canteen: " << std::endl;
	bserv::db_transaction tx{ conn };

	//选择该餐厅的窗口
	bserv::db_result db_res = tx.exec("SELECT count(*) from win where win.C_= ?;", canteen_num);
	lginfo << db_res.query();
	std::size_t total_wins = (*db_res.begin())[0].as<std::size_t>();
	lgdebug << "total wins: " << total_wins << std::endl;
	db_res = tx.exec("SELECT * from win where win.C_= ?;", canteen_num);
	lginfo << db_res.query();
	auto wins = orm_win.convert_to_vector(db_res);
	boost::json::array json_wins;
	for (auto& win : wins) {
		json_wins.push_back(win);
	}
	context["windows"] = json_wins;

	//选择该餐厅所拥有的标签
	db_res = tx.exec("SELECT count(*) from tag "
					"where exists( "
					"	select * from tag_belong, dish, win "
					"where tag_belong.T_ = tag.T_ and tag_belong.D_ = dish.D_ and dish.W_ = win.W_ and win.C_ = ?);"
					, canteen_num);
	lginfo << db_res.query();
	std::size_t total_tags = (*db_res.begin())[0].as<std::size_t>();
	lgdebug << "total tags: " << total_tags << std::endl;
	db_res = tx.exec( "SELECT * from tag "
					"where exists( "
					"	select * from tag_belong, dish, win "
					"where tag_belong.T_ = tag.T_ and tag_belong.D_ = dish.D_ and dish.W_ = win.W_ and win.C_ = ?);"
					, canteen_num);
	lginfo << db_res.query();
	auto tags = orm_tag.convert_to_vector(db_res);
	boost::json::array json_tags;
	for (auto& tag : tags) {
		json_tags.push_back(tag);
	}
	context["tags"] = json_tags;

	//选择该餐厅特定窗口、标签下的菜品
	if (table_num != 0)
	{
		if (tag_num != 0)
		{
			db_res = tx.exec("SELECT count(*) from dish "
				"where exists( "
				"	select * from tag_belong, win, tag "
				"where tag_belong.T_ = tag.T_ and tag_belong.D_ = dish.D_ and dish.W_ = win.W_ and win.C_ = ? and tag.T_ = ? and win.W_ = ? and dish.Dname like ? );"
				, canteen_num, tag_num, table_num, dish_search + "%");
			lginfo << db_res.query();
			std::size_t total_dishes = (*db_res.begin())[0].as<std::size_t>();
			lgdebug << "total dishes: " << total_dishes << std::endl;
			db_res = tx.exec("SELECT * from dish "
				"where exists( "
				"	select * from tag_belong, win, tag "
				"where tag_belong.T_ = tag.T_ and tag_belong.D_ = dish.D_ and dish.W_ = win.W_ and win.C_ = ? and tag.T_ = ? and win.W_ = ? and dish.Dname like ?);"
				, canteen_num, tag_num, table_num, dish_search + "%");
			lginfo << db_res.query();
		}
		else
		{
			db_res = tx.exec("SELECT count(*) from dish "
				"where exists( "
				"	select * from win "
				"where dish.W_ = win.W_ and win.C_ = ? and win.W_ = ? and dish.Dname like ?);"
				, canteen_num, table_num, dish_search + "%");
			lginfo << db_res.query();
			std::size_t total_dishes = (*db_res.begin())[0].as<std::size_t>();
			lgdebug << "total dishes: " << total_dishes << std::endl;
			db_res = tx.exec("SELECT * from dish "
				"where exists( "
				"	select * from win "
				"where dish.W_ = win.W_ and win.C_ = ? and win.W_ = ? and dish.Dname like ?);"
				, canteen_num, table_num, dish_search + "%");
			lginfo << db_res.query();
		}
	}
	else
	{
		if (tag_num != 0)
		{
			db_res = tx.exec("SELECT count(*) from dish "
				"where exists( "
				"	select * from tag_belong, win, tag "
				"where tag_belong.T_ = tag.T_ and tag_belong.D_ = dish.D_ and dish.W_ = win.W_ and win.C_ = ? and tag.T_ = ? and dish.Dname like ?);"
				, canteen_num, tag_num, dish_search + "%");
			lginfo << db_res.query();
			std::size_t total_dishes = (*db_res.begin())[0].as<std::size_t>();
			lgdebug << "total dishes: " << total_dishes << std::endl;
			db_res = tx.exec("SELECT * from dish "
				"where exists( "
				"	select * from tag_belong, win, tag "
				"where tag_belong.T_ = tag.T_ and tag_belong.D_ = dish.D_ and dish.W_ = win.W_ and win.C_ = ? and tag.T_ = ? and dish.Dname like ?);"
				, canteen_num, tag_num, dish_search + "%");
			lginfo << db_res.query();
		}
		else
		{
			db_res = tx.exec("SELECT count(*) from dish "
				"where exists( "
				"	select * from win "
				"where dish.W_ = win.W_ and win.C_ = ? and dish.Dname like ?);"
				, canteen_num, dish_search + "%");
			lginfo << db_res.query();
			std::size_t total_dishes = (*db_res.begin())[0].as<std::size_t>();
			lgdebug << "total dishes: " << total_dishes << std::endl;
			db_res = tx.exec("SELECT * from dish "
				"where exists( "
				"	select * from win "
				"where dish.W_ = win.W_ and win.C_ = ? and dish.Dname like ?);"
				, canteen_num, dish_search + "%");
			lginfo << db_res.query();
		}
	}
	

	auto dishes = orm_dish.convert_to_vector(db_res);
	boost::json::array json_dishes;
	for (auto& dish : dishes) {
		json_dishes.push_back(dish);
	}
	context["dishes"] = json_dishes;


	return index("dishes.html", session_ptr, response, context);
}

std::nullopt_t view_users(
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response,
	const std::string& page_num) {
	int page_id = std::stoi(page_num);
	boost::json::object context;
	return redirect_to_users(conn, session_ptr, response, page_id, std::move(context));
}

std::nullopt_t canteen_management(
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response,
	const std::string& page_num) {
	int page_id = std::stoi(page_num);
	boost::json::object context;
	return redirect_to_canteen(conn, session_ptr, response, page_id, std::move(context));
}

std::nullopt_t window_management(
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response,
	const std::string& page_num) {
	int page_id = std::stoi(page_num);
	boost::json::object context;
	return redirect_to_window(conn, session_ptr, response, page_id, std::move(context));
}

std::nullopt_t dish_management(
	std::shared_ptr<bserv::db_connection> conn,
	boost::json::object&& params,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response,
	const std::string& page_num) {
	int page_id = std::stoi(page_num);
	boost::json::object context;

	boost::json::object&& params_tmp = std::move(params);
	std::string D_tmp;
	if (!params_tmp["Dname_search"].is_string())
		D_tmp = "";
	else
		D_tmp = params_tmp["Dname_search"].as_string().c_str();

	return redirect_to_dish_search(conn, session_ptr, response, page_id, std::move(context), D_tmp);
}

std::nullopt_t tag_management(
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response,
	const std::string& page_num) {
	int page_id = std::stoi(page_num);
	boost::json::object context;
	return redirect_to_tag(conn, session_ptr, response, page_id, std::move(context));
}

std::nullopt_t dish_tag(
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response,
	const std::string& dish_num,
	const std::string& page_num) {
	int page_id = std::stoi(page_num);
	int dish_id = std::stoi(dish_num);
	boost::json::object context;
	return redirect_to_dish_tag(conn, session_ptr, response, page_id, dish_id, std::move(context));
}

std::nullopt_t form_add_user(
	bserv::request_type& request,
	bserv::response_type& response,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr) {
	boost::json::object context = user_register(request, std::move(params), conn);
	return redirect_to_users_login(conn, session_ptr, response, 1, std::move(context));
}

std::nullopt_t form_add_canteen(
	bserv::request_type& request,
	bserv::response_type& response,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr) {
	boost::json::object context = add_canteen_register(request, std::move(params), conn);
	return redirect_to_canteen(conn, session_ptr, response, 1, std::move(context));
}

std::nullopt_t form_add_window(
	bserv::request_type& request,
	bserv::response_type& response,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr) {
	boost::json::object context = add_window_register(request, std::move(params), conn);
	return redirect_to_window(conn, session_ptr, response, 1, std::move(context));
}

std::nullopt_t form_add_dish(
	bserv::request_type& request,
	bserv::response_type& response,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr) {
	boost::json::object context = add_dish_register(request, std::move(params), conn);
	return redirect_to_dish(conn, session_ptr, response, 1, std::move(context));
}

std::nullopt_t form_add_tag(
	bserv::request_type& request,
	bserv::response_type& response,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr) {
	boost::json::object context = add_tag_register(request, std::move(params), conn);
	return redirect_to_tag(conn, session_ptr, response, 1, std::move(context));
}

std::nullopt_t form_add_dish_tag(
	bserv::request_type& request,
	bserv::response_type& response,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr) {
	boost::json::object context = add_dish_tag_register(request, std::move(params), conn);
	boost::json::object&& params_tmp = std::move(params);
	int D_tmp = atof(params_tmp["D_"].as_string().c_str());
	return redirect_to_dish_tag(conn, session_ptr, response, 1, D_tmp, std::move(context));
}

std::nullopt_t delete_canteen(
	bserv::request_type& request,
	bserv::response_type& response,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr) {
	boost::json::object context = delete_canteen_from_database(request, std::move(params), conn);
	return redirect_to_canteen(conn, session_ptr, response, 1, std::move(context));
}

std::nullopt_t delete_window(
	bserv::request_type& request,
	bserv::response_type& response,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr) {
	boost::json::object context = delete_window_from_database(request, std::move(params), conn);
	return redirect_to_window(conn, session_ptr, response, 1, std::move(context));
}

std::nullopt_t delete_dish(
	bserv::request_type& request,
	bserv::response_type& response,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr) {
	boost::json::object context = delete_dish_from_database(request, std::move(params), conn);
	std::cout << "第一步完成" << std::endl;
	return redirect_to_dish(conn, session_ptr, response, 1, std::move(context));
}

std::nullopt_t delete_tag(
	bserv::request_type& request,
	bserv::response_type& response,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr) {
	boost::json::object context = delete_tag_from_database(request, std::move(params), conn);
	return redirect_to_tag(conn, session_ptr, response, 1, std::move(context));
}

std::nullopt_t delete_dish_tag(
	bserv::request_type& request,
	bserv::response_type& response,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr) {
	boost::json::object context = delete_dish_tag_from_database(request, std::move(params), conn);
	boost::json::object&& params_tmp = std::move(params);
	int D_tmp = atof(params_tmp["D_"].as_string().c_str());
	return redirect_to_dish_tag(conn, session_ptr, response, 1, D_tmp, std::move(context));
}

std::nullopt_t delete_remark(
	bserv::request_type& request,
	bserv::response_type& response,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr) {
	boost::json::object context = delete_remark_from_database(request, std::move(params), conn);
	return redirect_to_dish(conn, session_ptr, response, 1, std::move(context));
}

std::nullopt_t delete_user(
	bserv::request_type& request,
	bserv::response_type& response,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr) {
	boost::json::object context = delete_user_from_database(request, std::move(params), conn);
	return redirect_to_users(conn, session_ptr, response, 1, std::move(context));
}

std::nullopt_t update_canteen(
	bserv::request_type& request,
	bserv::response_type& response,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr) {
	boost::json::object context = update_canteen_from_database(request, std::move(params), conn);
	return redirect_to_canteen(conn, session_ptr, response, 1, std::move(context));
}

std::nullopt_t update_window(
	bserv::request_type& request,
	bserv::response_type& response,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr) {
	boost::json::object context = update_window_from_database(request, std::move(params), conn);
	return redirect_to_window(conn, session_ptr, response, 1, std::move(context));
}

std::nullopt_t update_dish(
	bserv::request_type& request,
	bserv::response_type& response,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr) {
	boost::json::object context = update_dish_from_database(request, std::move(params), conn);
	return redirect_to_dish(conn, session_ptr, response, 1, std::move(context));
}

std::nullopt_t update_tag(
	bserv::request_type& request,
	bserv::response_type& response,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr) {
	boost::json::object context = update_tag_from_database(request, std::move(params), conn);
	return redirect_to_tag(conn, session_ptr, response, 1, std::move(context));
}

std::nullopt_t form_add_remark(
	bserv::request_type& request,
	bserv::response_type& response,
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr,
	const std::string& canteen_num,
	const std::string& table_num,
	const std::string& tag_num,
	const std::string& dish_num) {
	int canteen_id = std::stoi(canteen_num);
	int table_id = std::stoi(table_num);
	int tag_id = std::stoi(tag_num);
	int dish_id = std::stoi(dish_num);

	bserv::session_type& session = *session_ptr;
	auto id = session["user"].as_object()["id"].as_int64();
	

	boost::json::object context = add_remark_to_database(request, std::move(params), conn, id, dish_id);
	return dish_content(conn, session_ptr, response, canteen_num, table_num, tag_num, dish_num);
}

boost::json::object add_remark_to_database(
	bserv::request_type& request,
	// the json object is obtained from the request body,
	// as well as the url parameters
	boost::json::object&& params,
	std::shared_ptr<bserv::db_connection> conn,
	int id,
	int dish_id) {
	//if (request.method() != boost::beast::http::verb::post) {
	//	throw bserv::url_not_found_exception{};
	//}
	//if (params.count("username") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`username` is required"}
	//	};
	//}
	//if (params.count("password") == 0) {
	//	return {
	//		{"success", false},
	//		{"message", "`password` is required"}
	//	};
	//}
	auto Rcontext = params["Rcontext"].as_string();
	int Rmark = atof(params["Rmark"].as_string().c_str());
	bserv::db_transaction tx{ conn };
	bserv::db_result r = tx.exec(
		"insert into ? "
		"(Rcontext, Rmark, id, D_) "
		"values "
		"(?, ?, ?, ?)", bserv::db_name("remark"),
		Rcontext,
		Rmark,
		id,
		dish_id);
	lginfo << r.query();
	tx.commit(); // you must manually commit changes
	return {
		{"success", true},
		{"message", "user registered"}
	};
}

std::nullopt_t canteen_index(
	std::shared_ptr<bserv::db_connection> conn,
	boost::json::object&& params,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response,
	const std::string& canteen_num,
	const std::string& table_num, 
	const std::string& tag_num) {
	int canteen_id = std::stoi(canteen_num);
	int table_id = std::stoi(table_num);
	int tag_id = std::stoi(tag_num);
	boost::json::object context;

	boost::json::object&& params_tmp = std::move(params);
	std::string D_tmp;
	if (!params_tmp["Dname_search"].is_string())
		D_tmp = "";
	else
		D_tmp = params_tmp["Dname_search"].as_string().c_str();
	return redirect_to_canteen_index(conn, session_ptr, response, std::move(context), canteen_id, table_id, tag_id, D_tmp);
}

std::nullopt_t dish_content(
	std::shared_ptr<bserv::db_connection> conn,
	std::shared_ptr<bserv::session_type> session_ptr,
	bserv::response_type& response,
	const std::string& canteen_num,
	const std::string& table_num,
	const std::string& tag_num,
	const std::string& dish_num) {
	int canteen_id = std::stoi(canteen_num);
	int table_id = std::stoi(table_num);
	int tag_id = std::stoi(tag_num);
	int dish_id = std::stoi(dish_num);
	boost::json::object context;
	std::cout << "进入" << std::endl;
	return redirect_to_dish(conn, session_ptr, response, std::move(context), canteen_id, table_id, tag_id, dish_id);
}

