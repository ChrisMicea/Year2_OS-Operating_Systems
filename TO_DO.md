modific logica de la view astfel incat sa caute dupa district ID, nu sa dea jump cu atatea pozitii cate primeste ca si parametru
    exemplu: pt downtown, cum arata acum, daca dai view downtown 3 nu iti va gasi, pt ca raportul cu ID 3 e pe pozitia 1 => da jump cu offset 3 si nu gaseste

adaug aceleasi verificari ca si in create_district din district.c, legat de dimensiunea comenzii (districtID) + dimensiunea a relative_filepath sa fie mai mica ca si buffer PESTE TOT !!!!!!!!!!!!!!!!!!!!

modific logica din city_hub -> calculate_scores() asa incat cand da fail orice fork() sau pipe() sa dea exit frumos programul

--- sa adaug un print function comun pt code duplication in view_specific_report, list_reports si filter_reports
--- sa fac consistent return type-urile la toate functiile, folosesc enum
--- in view, list (si altele, daca e cazul), sa incerc sa ma folosesc mai intai de symlink, abia apoi de directory path, ca in add_report
--- scan, detect and report dangling symbolic links
--- update ai_usage.md maybe if first lines of description / storytelling are not enough
