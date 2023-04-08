CREATE TABLE auth_user (
    id serial PRIMARY KEY,
    username character varying(255) NOT NULL UNIQUE,
    password character varying(255) NOT NULL,
    is_superuser boolean NOT NULL,
    first_name character varying(255) NOT NULL,
    last_name character varying(255) NOT NULL,
    email character varying(255) NOT NULL,
    is_active boolean NOT NULL
);

CREATE TABLE canteen(
    C_ integer PRIMARY KEY,
    Cname character varying(255) NOT NULL UNIQUE,
    Cpicture character varying(255) 
);

CREATE TABLE win(
    W_ serial PRIMARY KEY,
    Wname character varying(255) NOT NULL,
    Wlocation character varying(255),
    C_ integer,
    FOREIGN KEY(C_) REFERENCES canteen(C_)
);

CREATE TABLE dish(
    D_ serial PRIMARY KEY,
    Dname character varying(255) NOT NULL,
    Dprice real,
    is_sell boolean NOT NULL,
    Dpicture character varying(255) ,
    W_ integer,
    FOREIGN KEY(W_) REFERENCES win(W_)
);

CREATE TABLE remark(
    R_ serial PRIMARY KEY,
    Rcontext character varying(255),
    Rmark INTEGER,
    id integer,
    D_ integer,
    FOREIGN KEY(id) REFERENCES auth_user (id),
    FOREIGN KEY(D_) REFERENCES dish(D_)
);

CREATE TABLE tag(
    T_ serial PRIMARY KEY,
    Tname character varying(255),
    Tsupport integer
);

CREATE TABLE tag_belong(
    T_ integer,
    D_ integer,
    PRIMARY KEY(T_, D_),
    FOREIGN KEY(T_) REFERENCES tag(T_),
    FOREIGN KEY(D_) REFERENCES dish(D_)
);