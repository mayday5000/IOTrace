/*****************************************************************************/
/***                                                                       ***/
/*** Desarrollado por: Damian Andres Ulanowicz - 2017.                     ***/
/***                                                                       ***/
/*** Version: 1.00                                                         ***/
/***                                                                       ***/
/*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#define MAX_NAME_LEN 80
#define MAX_LINE_LEN 1023
#define ORDER_ASC 0
#define ORDER_DES 1


class node
{
public:
	node();
	long id;
    char name[MAX_NAME_LEN+1];
	unsigned short rep_count;
	node *next;
	~node(){};
};

class sort_engine
{
private:
    node *top, *index;

//    void swap_node(node *xp, node *yp);
    void swap_node(node *xp, node *yp);

public:
    sort_engine();
    int add_data(long id, char *name);
    void sort_data(int order);
    void go_top(void);
    char *read_data(long *id);
    ~sort_engine(){};
};

node::node()
{
    id = 0;
    rep_count = 0;
    memset(name,0,sizeof(char)*(MAX_NAME_LEN+1));
    next = NULL;
}

sort_engine::sort_engine()
{
    top = NULL;
    index = top;
}

/*void sort_engine::swap_node(node *xp, node *yp)
{
    node *aux = xp;

    aux = xp;
    xp = yp;
    yp = aux;

//    *xp = *yp;
//    *yp = aux;
}*/


/*void sort_engine::swap_node(node **xp, node **yp)
{
    node *aux = *xp;

    *xp = *yp;
    *yp = aux;
}*/

void sort_engine::swap_node(node *xp, node *yp)
{
    node aux = *xp;
    node *next_a, *next_b;

    next_a = xp->next;
    next_b = yp->next;
    *xp = *yp;
    *yp = aux;
    xp->next = next_a;
    yp->next = next_b;
}


int sort_engine::add_data(long id, char *name)
{
    int ret=-1;
    int name_len=0;
    int fg_match_id=0;

    if(!name) return(ret);
    if(!top)
    {
        top = new node();
        if(top)
        {
            name_len = strlen(name);
            if(name_len > MAX_NAME_LEN)
                name_len = MAX_NAME_LEN;          // se trunca el string a la longitud maxima.
            memcpy(top->name, name, name_len);
            top->name[name_len] = 0;

            top->next = NULL;
            top->id = id;
            index = top;
            ret=0;
        }
    }
    else
    {
        node *p = NULL, *p1 = NULL;
        for(p = top;p->next != NULL;p = p->next)
        {
            if(p->id == id)     // Existe un nodo en la lista con el mismo id?
            {
                fg_match_id = 1;
                p1 = p;         // p1 sera el nodo a actualizar rep_count
            }
        }
        if(p->id == id)         // Existe un nodo en la lista con el mismo id? (p -> ultimo nodo de la lista)
        {
            fg_match_id = 1;
            p1 = p;             // p1 sera el nodo a actualizar rep_count
        }
        if(fg_match_id == 0)    // si el id no existe, se agrega un nuevo nodo...
        {
            p1 = new node();
            if(p1 && p)
            {
                name_len = strlen(name);
                if(name_len > MAX_NAME_LEN)
                    name_len = MAX_NAME_LEN;          // se trunca el string a la longitud maxima.
                memcpy(p1->name, name, name_len);
                p1->name[name_len] = 0;

                p->next = p1;
                p1->next = NULL;
                p1->id = id;
                ret=0;
            }
        }
        else if(p1->rep_count < USHRT_MAX)
            p1->rep_count++;                          // Incrementa la cantidad de repeticiones del nodo.
    }

    return(ret);
}

void sort_engine::go_top(void)
{
    index = top;
}

void sort_engine::sort_data(int order)
{
    int fg_swap = 0;
    node *ptr, *last_ptr=NULL;

    do
    {
        fg_swap = 0;
        ptr = top;

        while (ptr->next != last_ptr)
        {
            if(order == ORDER_ASC)
            {
                if (ptr->rep_count > ptr->next->rep_count)
                {
                    swap_node(ptr, ptr->next);
                    fg_swap = 1;
                }
            }
            if(order == ORDER_DES)
            {
                if (ptr->rep_count < ptr->next->rep_count)
                {
                    swap_node(ptr, ptr->next);
                    fg_swap = 1;
                }
            }
            ptr = ptr->next;
        }
        last_ptr = ptr;
    }
    while (fg_swap);
}

char *sort_engine::read_data(long *id)
{
    char *ret=NULL;

    if(index)
    {
        *id = index->id;
        ret = index->name;
        index = index->next;
    }

    return(ret);
}

int main(int argc, char *argv[])
{
    int cnt=0, str_len=0, value_len=0;
    long id=0;
    FILE *fd = NULL;
    sort_engine *sort_eng=new sort_engine();
    char *name=NULL, *endptr=NULL;
    char buffer[MAX_LINE_LEN+1], *aux, value[MAX_LINE_LEN+1];

//	gets(buffer);   // Pausa para attachear debugger...
//    scanf("Esperando debugger...");

    argc = 2;
    argv[1] = (char*)malloc(100);
    strcpy(argv[1], "in.txt");


    if(argc < 2)
    {
	    printf("Uso: %s <archivo>\n", argv[0]);
        exit(0);
    }

    if(!(fd = fopen(argv[1], "r")))
    {
	    printf("Error: no se pudo abrir el archivo\n");
        exit(0);
    }

    fseek(fd, 0L, SEEK_SET);
    while(fgets(buffer, sizeof(buffer), fd) != NULL)
    {
        name=NULL;
        char *c = strchr(buffer, '\n');             // UNIX end-of-line '\n'
        if (c) *c = 0;
        c = strchr(buffer, '\r');                   // DOS end-of-line '\r\n'
        if (c) *c = 0;

        str_len=strlen(buffer);
        if(str_len > MAX_LINE_LEN) str_len = MAX_LINE_LEN;
        buffer[str_len] = 0;                        // Trunca el string
        if((aux=strchr(&buffer[0],' '))!=NULL)
        {
            value_len=(int)(&aux[0]-&buffer[0]);    // Posicion del caracter ' ' en el buffer
            if((value_len >= 2) && (value_len < str_len))
            {
                memcpy(&value[0], &buffer[0], value_len);
                value[value_len]=0;

                id = strtol(value, &endptr, 16);
                if (!((errno == ERANGE && (id == LONG_MAX || id == LONG_MIN)) || (errno != 0 && id == 0)))
                {
                    if(str_len > (value_len+1))
                        name = buffer+value_len+1;

                    if (endptr != value)
                    {
                        sort_eng->add_data(id, name);
                    }
                }
            }
        }
    }
    fclose(fd);

    /*sort_eng->add_data(1, "damian");
    sort_eng->add_data(0, "ariel");
    sort_eng->add_data(1, "damian");
    sort_eng->add_data(2, "diego");
    sort_eng->add_data(4, "cynthia");*/

    sort_eng->sort_data(ORDER_DES);
    while((name = sort_eng->read_data(&id)) && (cnt < 3))
    {
       printf("[+] Id: %ld - Nombre: %s\n", id, name);
       cnt++;
    }

    delete(sort_eng);
    return(0);
}
