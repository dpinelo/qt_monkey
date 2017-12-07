CREATE TABLE testsuites
(
  id integer PRIMARY KEY,
  name character varying(255) NOT NULL,
  executablepath character varying(1000) NOT NULL,
  arguments character varying(1000) NOT NULL
);

CREATE TABLE testsuitestest
(
  id integer PRIMARY KEY,
  idtestsuite integer,
  name character varying(255) NOT NULL,
  position integer,
  code text,
  FOREIGN KEY (idtestsuite) REFERENCES testsuites(id) ON UPDATE CASCADE ON DELETE CASCADE
);
