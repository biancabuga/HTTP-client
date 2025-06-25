# Buga Bianca Gabriela 321CD

# Tema 3 - Protocoale de Comunicații (PCOM)

Am implementat un client pentru o librarie de filme care permite utilizatorului sa interactioneze cu un server pentru gestionarea filmelor si a colectiilor de filme. In rezolvarea acestei teme, am pornit de la laboratorul 9.

Folosim biblioteca **parson** pentru parsarea JSON pentru limbajul C. Folosim aceasta biblioteca pentru a parsa JSON mesajele trimise la server sau mesajele primite de la server pe care trebuie sa le afisam.

## CLIENT.C

In main citim comenzile de la utilizator pentru a apela functiile corespunzatoare.

## FUNCTII - la fiecare comanda inchidem si deschidem conexiunea cu serverul

### 1. login_admin()
Aceasta functie deschide o conexiune cu serverul folosind functia open_connection configurata pe adresa si portul serverului. Folosind biblioteca parson, creez un obiect JSON in care se adauga username si password. Payload-ul este incorporat intr-o cerere POST, iar mesajul formatat se trimite la server, care de asemenea, ne trimite un raspuns pe care il primim cu receive_from_server(). Verificam daca cererea HTTP a fost procesata cu succes, adica daca raspunsul serverului contine statusul 200 Ok si daca avem un header pentru cookie. In caz afirmativ, extragem cookie-ul si il salvam intr-o variabila globala pe care o vom folosi ulterior la operatiunile de admin. Inchidem conexiunea.

### 2. login()
Functia citeste datele utilizatorului si se conecteaza la server, dupa care creeaza un payload json cu campurile citite. Construiesc o cerere POST catre server care contine payload-ul cu datele noastre si cookie-ul de la admniul sub care se face login-ul. Verificam raspunsul de la server si daca raspunsul contine "200 OK", cautam header-ul pentru cookie si extragem cookie-ul userului. Eroare 403 inseamna ca suntem deja logati ca admin si nu putem sa ne logam ca user si eroare 404 daca utilizatorul nu exista pentru adminul respectiv.

### 3. add_user()
Ca sa adaugam un user, trebuie sa fim logati ca admin, deci verificam cookie-ul de sesiune. Citim datele pentru utilizator si ne conectam la server. Creez payload JSON si construiesc o cerere POST catre url-ul corespunzator, precizez tipul de continut "application/json" si trimit payload si cookie-ul de admin. Procesam raspunsul, avand grija sa tratam cazurile speciale si erorile.

### 4. get.users()
Si aceasta comanda necesita sa fiu logat ca admin. Pornim conexiunea cu serverul dar de data asta nu mai avem payload pentru ca nu introducem date, doar cerem. Trimitem o cerere de tip GET in care punem si cookie-ul de admin si extragem din raspuns userii. Pentru fiecare user, extragem din json campurile pe care vrem sa le afisam

### 5. delete_user()
Pentru a sterge un user ca admin trebuie sa-i citim numele si construim un URL dinamic in care includem username ul, fara a mai fi nevoie sa creem payload. Trimitem cerere de tip DELETE cu url si cookie.

### 6. logout_admin()
Delogheaza adminul printr-o cerere GET catre server si goleste bufferul pentru cookie-ul adminului.

### 7. get_access()
Obtine un token JWT pentru accesul la biblioteca pentru un user. Facem o cerere de tip GET, trimitem si cookie-ul si raspunsul json il parsam si extragem valoarea campului "token" in bufferul jwt_token. 

### 8. logout()
Delogheaza un user, trimite o cerere GET si goleste bufferele pentru cookie-ul userului si pentru tokenul de acces.

### 9. get_movies()
Formam un header cu jwt_token pe care il trimitem in cererea GET catre server alaturi de cookie-ul userului. Extragem din raspuns filmele, care sunt in format json, le parsam si din fiecare film extragem campurile pe care vrem sa le afisam.

### 10. add_movie()
Citim datele unui film, apelam get_movies pentru a popula variabilele globale movie_count si movie_ids, si pentru fiecare film facem un url dinamic in care trimitem id ul filmului. Extragem headerul si cookie-urile si trimitem cerere get la URL. Parsam raspunsul primit si extragem campurile year si title pentru a verifica daca filmul pe care vrem sa l adaugam nu exista deja. Daca nu exista, adaugam filmul cu cerere cu payload etc.

### 11. get_movie()
Trebuie sa citim id ul filmului si sa avem acces, trimitem cerere GET cu token si cookie de user, printam json-ul filmului.

### 12. update_movie()
Similar ca la add_movie doar ca cerem datele pe care trebuie sa le punem in locul datelor unui film deja existent.

### 13. delete_movie()
Similar ca la get_movie doar ca trimitem o cerere de tip DELETE.

### 14. get_collections()
Returneaza colectiile unui user. Trimite o cerere GET la server cu cookie de user si token de acces. Parsam raspunsul primit luand din el colectiile si numarul lor. Pentru fiecare colectie afusam datele preluate din json.

### 15. add_collection()
Adaugam o colectie - citim indexul filmelor din lista de filme adaugate, pe care le contine colectia.

### 16. add_movie_to_collection()
Adauga filmele cu indexul respectiv in colectie.

### 17. get_collection()
Obtine o anumita colectie.

### 18. delete_collection()
Sterge colectia

### 19. delete_movie_from_collection
Sterge filmele dintr-o colectie.

## request.c

In acest fisier avem functii pentru a crea si trimite cereri HTTP

