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

-- 食堂：
CREATE TABLE canteen(
    C_ integer PRIMARY KEY,
    Cname character varying(255) NOT NULL UNIQUE,
    Cpicture character varying(255) 
);

INSERT INTO canteen
    (C_, Cname, Cpicture)
    VALUES( 1,'北食', '/1');

update canteen set row = ()

-- 窗口：
CREATE TABLE win(
    W_ serial PRIMARY KEY,
    Wname character varying(255) NOT NULL,
    Wlocation character varying(255),
    C_ integer,
    FOREIGN KEY(C_) REFERENCES canteen(C_)
);

INSERT INTO win
    VALUES(5, '中式小吃', '二楼', 2);

insert into win(Wname, Wlocation, C_) 
values('早餐', '一楼', (select C_ from canteen where Cname = '南食') );

delete from win where W_ = 1;


update win set Wname = '中式小吃', Wlocation = '二楼', 
C_ = (select C_ from canteen where Cname = '旦院') where W_ = 5;

-- 菜品：
CREATE TABLE dish(
    D_ serial PRIMARY KEY,
    Dname character varying(255) NOT NULL,
    Dprice real,
    is_sell boolean NOT NULL,
    Dpicture character varying(255) ,
    W_ integer,
    FOREIGN KEY(W_) REFERENCES win(W_)
);

INSERT INTO dish
    VALUES(16, '小馄饨', 8, true, '/16', 5);

INSERT into dish(Dname, Dprice, is_sell, Dpicture, W_) values('鸡腿', 4.5, TRUE, '/17', (select win.W_ from win, canteen where win.C_=canteen.C_ and win.Wname = '大中餐' and canteen.Cname='南食') );

 (select win.W_ from win, canteen where win.C_=canteen.C_ and win.Wname = '')


-- 评论：
CREATE TABLE remark(
    R_ serial PRIMARY KEY,
    Rcontext character varying(255),
    Rmark INTEGER,
    id integer,
    D_ integer,
    FOREIGN KEY(id) REFERENCES auth_user (id),
    FOREIGN KEY(D_) REFERENCES dish(D_)
);

INSERT INTO remark
    (Rcontext, Rmark, id, D_)
    VALUES( '酸汤肥牛太好吃啦！！强烈推荐QAQ', 95, 1, 6);

-- 标签：
CREATE TABLE tag(
    T_ serial PRIMARY KEY,
    Tname character varying(255),
    Tsupport integer
);

INSERT INTO tag
    VALUES(1, '川菜', 1);

-- 标签从属关系：
CREATE TABLE tag_belong(
    T_ integer,
    D_ integer,
    PRIMARY KEY(T_, D_),
    FOREIGN KEY(T_) REFERENCES tag(T_),
    FOREIGN KEY(D_) REFERENCES dish(D_)
);

INSERT INTO tag_belong
    VALUES(1, 6);


展示dish目录查询代码：
1）所有窗口：

SELECT Wname from win where win.C_= canteen_num;

2)所有tag：
SELECT tag.T_ from tag
where exist(
    select * from tag_belong, dish 
    where tag_belong.T_=tag.T_ and tag_belong.D_ = dish.D_ and dish.W_= window_num;
)

