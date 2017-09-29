//Falta por solucionar: 

/*
Autor: Ricardo Andrey Sánchez Delgado
email: s.4ndrey@gmail.com

*/
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>


#define blocks 2048

struct table{
	int libres[blocks][2];
	int ocupados[blocks][2];
	char archivos[blocks][256];
	int s_ocupados;
	int s_libres;
	int base;
	int limite;
};

int inicializarEstructura(struct table *tab){
	tab->s_ocupados = 0;
	tab->s_libres = 0;
	tab->base = sizeof(struct table);;
	tab->limite = sizeof(struct table);
}


int indiceLibre(struct table *tab, int tamano){
	int i;
	int temp = 0;
	for(i=0;i<tab->s_libres;i++){
		temp = tab->libres[i][1] - tab->libres[i][0];
		if(tamano<= temp){
			return i;
		}
	}
	return -1;
}

//Se inserta el archivo en un hueco en especifico
int aux_insertarArchivo(struct table *tab, int tamano, int indice, char *nombreArchivo){
	//tab->s_libres--;
	int base = tab->libres[indice][0];
	int limite = tab->libres[indice][1];
	tab->ocupados[tab->s_ocupados][0] = base;
	tab->ocupados[tab->s_ocupados][1] = base  + tamano;
	strcpy(tab->archivos[tab->s_ocupados], nombreArchivo);
	if(limite){ //Si sobra espacio se asigna otro espacio libre
		tab->libres[tab->s_libres][0] = base  + tamano;
		tab->libres[tab->s_libres][1] = limite;
		tab->s_libres++;		
	}

	reacomodarLista(tab, 1, indice);

	tab->s_ocupados++;

}

int desfragmentar(struct table *tab, char *filename, char v, char d){
	FILE *fp;
	fp = fopen(filename, "r+");
	if(v)
		printf("Apertura de archivo: %s\n", filename);
	//rewind();
	if(v){
		printf("Se carga la tabla de direcciones del archivo tar\n");
	}
	fread(tab, sizeof(struct table), 1, fp);

	if(d){
		printf("---Tabla de valores antes de la desfragmentacion--\n");
		imprimirValores(tab);
	}

	int i;
	int e;
	int j;
	int diferencia;
	char temp;
	for(i = 0;i<tab->s_libres;i++){
		if(tab->libres[i][0]<tab->base){
			continue;
		}
		diferencia = tab->libres[i][1] - tab->libres[i][0];
		for(e = 0;e<tab->s_ocupados;e++){
			if(tab->ocupados[e][0]>=tab->libres[i][1]){
				if(v){
					printf("Direccion anterior de: %s, base: %d, limite: %d\n", tab->archivos[e], tab->ocupados[e][0]-tab->base, tab->ocupados[e][1]-tab->base);
				}
				tab->ocupados[e][0] = tab->ocupados[e][0] - diferencia;
				tab->ocupados[e][1] = tab->ocupados[e][1] - diferencia;
				if(v){
					printf("Direccion actualizada de: %s, base: %d, limite: %d\n", tab->archivos[e], tab->ocupados[e][0] - tab->base, tab->ocupados[e][1]-tab->base);
				}
				for(int j = 0;j<tab->ocupados[e][1] - tab->ocupados[e][0];j++){
					fseek( fp, tab->ocupados[e][0] + diferencia +j, SEEK_SET );
					fread(&temp, sizeof(char), 1, fp);
					fseek( fp, tab->ocupados[e][0] +j, SEEK_SET );
					fwrite( &temp, sizeof(char), 1, fp); //Se escribe en el nuevo archivo
				}
			}
		}
	}
	tab->s_libres = 0;
	fseek( fp, 0, SEEK_SET ); // Se reescribe la tabla de direcciones
	fwrite( tab, sizeof(struct table), 1, fp);
	fclose(fp);

	if(d){
		printf("---Tabla de valores despues de la desfragmentacion--\n");
		imprimirValores(tab);
	}
}


int insertarArchivo(struct table *tab, char *nombreArchivo, int tamano, char v){
	int indice = indiceLibre(tab, tamano);
	if(indice<0){
		unirHuecos(tab);	//Falta reaccomodar los huecos a nivel fisico
	}
	indice = indiceLibre(tab, tamano);
	if(indice < 0){// Se debe de hacer el archivo mas grande
		if(v)
			printf("No hay espacio disponible para asignar, redimensionando archivo\n");
		tab->ocupados[tab->s_ocupados][0] = tab->limite; //Se establece la base del nuevo archivo
		tab->ocupados[tab->s_ocupados][1] = tab->limite + tamano; //Se establece el limite del nuevo archivo
		tab->limite = tab->limite + tamano;
		strcpy(tab->archivos[tab->s_ocupados], nombreArchivo);
		tab->s_ocupados++;
	}else{// Se inserta el archivo en ese hueco
		aux_insertarArchivo(tab, tamano, indice, nombreArchivo);
		//printf("Se deberia de aprovechar el hueco: %d\n", indice);
	}
	return 0;
}
 
void imprimirValores(struct table *tab){
	printf("----Resumen----\n");
	printf("Nota: Para las direcciones que se presentan en los bloques, hace falta sumar la base de la tabla para obtener la direccion real\n");
	printf("Libres: \n");
	int i;
	for(i=0;i<tab->s_libres;i++){
		printf("i = %d base: %d limite: %d\n",i ,tab->libres[i][0] - tab->base, tab->libres[i][1] - tab->base);
	}
	printf("------------\n");

	printf("Ocupados: \n");
	printf("tab_ocupados: %d\n", tab->s_ocupados);
	for(i=0;i<tab->s_ocupados;i++){
		printf("nom = %s base: %d limite: %d\n",tab->archivos[i] ,tab->ocupados[i][0] - tab->base, tab->ocupados[i][1] - tab->base);
	}
	printf("------------\n");
	printf("s_ocupados: %d s_libres: %d base: %d limite: %d\n", tab->s_ocupados, tab->s_libres, tab->base, tab->limite);

	printf("----Fin del resumen----\n");
}

int indiceOcupado(struct table *tab, char *nombreArchivo){
	int i;
	for(i=0;i<tab->s_ocupados;i++){
		if(strcmp(nombreArchivo, tab->archivos[i]) == 0){
			return i;
		}
	}
	return -1;
}

int asignarLibre(struct table *tab, int base, int limite){
	tab->libres[tab->s_libres][0] = base;
	tab->libres[tab->s_libres][1] = limite;
	tab->s_libres++;
	return 0;
}

int eliminarArchivo(struct table *tab, char *nombreArchivo){
	int indice = indiceOcupado(tab, nombreArchivo);
	if(indice == -1){
		printf("no se puede eliminar: %s\n", nombreArchivo);
		return 0;
	}
	int base = tab->ocupados[indice][0];
	int limite = tab->ocupados[indice][1];
	reacomodarLista(tab, 0, indice);
	asignarLibre(tab, base, limite);
}

int unirHuecos(struct table *tab){
	int i;
	for(i=0;i<tab->s_libres;i++){
		if(tab->libres[i][1] == tab->libres[i+1][0]){
			tab->libres[i][1] = tab->libres[i+1][1];
			reacomodarLista(tab, 1, i+1);
		}
	}

	return 0;
}


int reacomodarLista(struct table *tab, int libre, int indice){
	int i;
	if(libre == 1){
		for(i=indice;i<tab->s_libres;i++){
			tab->libres[i][0] = tab->libres[i+1][0];
			tab->libres[i][1] = tab->libres[i+1][1];
		}
		tab->s_libres--;
		return 0;
	}else{
		for(i=indice;i<tab->s_ocupados;i++){
			tab->ocupados[i][0] = tab->ocupados[i+1][0];
			tab->ocupados[i][1] = tab->ocupados[i+1][1];
			strcpy(tab->archivos[i], tab->archivos[i+1]);
		}
		tab->s_ocupados--;
		return 0;
	}
	printf("Error, no se ha reacomodado ninguna lista\n");
	return -1;
}

int getSize(char *filename){
	struct stat st;
	stat(filename, &st);
	return st.st_size;
}


int pathRelativo(char *string){
	int i;
	int pivote = 0;
	//printf("entro a path relativo\n");
	for(i=0;i<strlen(string)-1;i++){
		if(string[i] == '/'){
			//printf("pivote: %d nombre: %s\n", i+1, string);
			pivote = i+1;
		}
	}
	if(pivote==0){
		return 0;
	}
	for(i=0;i + pivote<strlen(string);i++){
		string[i] = string[i+pivote];
	}
	string[pivote] = '\0';
	return 0;
}


int getUltimoElemDir(struct table *tab, char *carp){
	char ind = 0;
	for(int i =0;i<tab->s_ocupados;i++){
		if(strcmp(tab->archivos, carp) == 0){
			ind = 1;
		}
		if(ind == 1 && tab->archivos[i][0]=='['){
			return i;
		}
	}
	return -1;
}

int mover(struct table *tab, int posiciones, int indice){
	for(int i=tab->s_ocupados-1;i>=indice;i--){
		strcpy(tab->archivos[i+1], tab->archivos[i]);
		tab->ocupados[i+1][0] = tab->ocupados[i][0];
		tab->ocupados[i+1][1] = tab->ocupados[i][1];
	}
	tab->s_ocupados++;
}

int insertarUltimoEnPos(struct table *tab, int indice){
	int base = tab->ocupados[tab->s_ocupados-1][0];
	int limite = tab->ocupados[tab->s_ocupados-1][1];
	char nombre[256];
	strcpy(nombre, tab->archivos[tab->s_ocupados-1]);
	tab->s_ocupados--;
	tab->ocupados[indice][0] = base;
	tab->ocupados[indice][1] = limite;
	strcpy(tab->archivos[indice], nombre);

}

int listdir(struct table *tab, const char *name, int level, char v){
	//pathRelativo(name);
	DIR *dir;
	struct dirent *entry;
	/*Copia del nombre*/
	char t[256];
	strcpy(t, name);

	/*String de la carpeta a la que pertenece*/
	char carp[256] = "[";
	strcat(carp, name);
	strcat(carp, "/");
	strcat(carp, "]");
	//printf("se va a insertar: %s\n", name);
	if((dir = opendir(name))){
		/*Se inserta un nuevo registro aunque es una carpeta para mantener la consistencia del modelo*/
		int indiceOcupado(tab, carp);
		tab->ocupados[tab->s_ocupados][0] = 0; //Se establece la base del nuevo archivo
		tab->ocupados[tab->s_ocupados][1] = 0; //Se establece el limite del nuevo archivo
		strcpy(tab->archivos[tab->s_ocupados], carp);
		tab->s_ocupados++;
		//return 0;
	/* ************* */
	}else{
		int indice = indiceOcupado(tab, "[./]");
		if(indice == -1){
			tab->ocupados[tab->s_ocupados][0] = 0; //Se establece la base del nuevo archivo
			tab->ocupados[tab->s_ocupados][1] = 0; //Se establece el limite del nuevo archivo
			strcpy(tab->archivos[tab->s_ocupados], "[./]");
			tab->s_ocupados++;
		}

		indice = getUltimoElemDir(tab, "[./]");
		insertarArchivo(tab, name, getSize(name),v);
		mover(tab, 1, indice+1);
		insertarUltimoEnPos(tab, indice+1);
		return 0;
	}
	if(!(entry = readdir(dir))){
		return -1;
	}
	do{
		if(entry->d_type!=DT_DIR){
			if(v)
				printf("%*s- %s\n", level*2, "", entry->d_name);
			strcat(t,"/");
			strcat(t, entry->d_name);
			insertarArchivo(tab, entry->d_name, getSize(t),v);

		}


	}while(entry = readdir(dir));


	if((dir = opendir(name))){
		/*Se carga de nuevo el directorio*/
	}

	if(!(entry = readdir(dir))){
		return -1;
	}
	do{
		if(entry->d_type==DT_DIR){
			char path[1024];
			int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
			path[len] = 0;
			if(strcmp(entry->d_name, ".")==0 || strcmp(entry->d_name,"..")==0)
				continue;
			if(v)
				printf("%*s[%s]\n", level*2, "",entry->d_name);
			tab->ocupados[tab->s_ocupados][0] = 0; //Se establece la base del nuevo archivo
			tab->ocupados[tab->s_ocupados][1] = 0; //Se establece el limite del nuevo archivo
			strcpy(tab->archivos[tab->s_ocupados], entry->d_name);
			listdir(tab, path, level + 1, v);

		}


	}while(entry = readdir(dir));



	closedir(dir);
	return 0;
}

int crearCarpetas(char *string){
    char temp[256]="";
    int piv = 0;
    int i =0;
    for(piv=0;piv<strlen(string);piv++){
        if(string[piv] == '/'){
            temp[i] = string[piv];
            //temp[i+1] = '\0';
            printf("ruta: %s\n", temp);
            mkdir(temp,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            //temp[i+1] = 'a';
            //i=0;
            i++;
            continue;
        }
        temp[i] = string[piv];
        i++;
    }
    return 0;

}

int normalizarStringCarpeta(char *string){
    int i;
    for(i=0;i<strlen(string)-1;i++){
        string[i] = string[i+1];
    }
    string[strlen(string)-2] = '\0';
    return 0;
}


int xtractTar(struct table *tab, char *filename, char v, char d){
	FILE *fp;
	FILE *fq;
	fp = fopen(filename, "r+");
	if(v)
		printf("Apertura de archivo: %s\n", filename);
	//rewind();
	fread(tab, sizeof(struct table), 1, fp);
	if(d){
		printf("Cargando tabla de direcciones\nDetalles: ");
		imprimirValores(tab);
	}
	int i;
	int e;
	int indice;
	char temp;
	char prefijo[1000];
	char ruta[1500];

	for(i = 0;i<tab->s_ocupados;i++){
		printf("file: %s\n", tab->archivos[i]);
		indice = indiceOcupado(tab, tab->archivos[i]);
		if(tab->archivos[i][0]=='['){
			strcpy(prefijo, tab->archivos[i]);
			normalizarStringCarpeta(prefijo);
			continue;
		}
		strcpy(ruta,prefijo);
		strcat(ruta, tab->archivos[i]);
		if(indice==-1){
			printf("Error: No existe el achivo solicitado\n");
			return -1;
		}
		crearCarpetas(ruta);
		fq=fopen(ruta,"w+"); // Se carga a memoria el archivo que se quiere cargar
		if(v)
			printf("Definiendo direccion base: %d y limite: %d\n", tab->ocupados[indice][0], tab->ocupados[indice][1]);
			fseek( fp, tab->ocupados[indice][0], SEEK_SET );
		for(e = tab->ocupados[indice][0];e<tab->ocupados[indice][1];e++){
			fread(&temp, sizeof(char), 1, fp);	//Se lee los datos del archivo que se quiere empaquetar
    		fwrite( &temp, sizeof(char), 1, fq); //Se escribe en el nuevo archivo
		}
		fclose(fq);
	}
	fclose(fp);
	return 0;
}

//The available access modes are
//O_RDONLY    O_WRONLY     O_RDWR    O_APPEND    O_BINARY    O_TEXT
int writeTar(struct table *tab, char *filename, char *files[], int numFiles, char append, char v, char d){
	FILE *fp;
	FILE *fq;
	if(append){
		fp =  fopen(filename,"r+");
		if(v)
			printf("El archivo se abrio en modo Append\n");
	}else{
		fp=fopen(filename,"w+");
		if(v)
			printf("Se creara un nuevo archivo\n");
	}
	int i;
	for(i = 0;i<numFiles;i++){
		if(v)
			printf("Insertando: %s\n", files[i]);
			listdir(tab, files[i], 0, v);
	}

	if(d)
		imprimirValores(tab);
	fwrite( tab, sizeof(struct table), 1, fp);
	if(v)
		printf("Se guardo la tabla de direcciones en el archivo, tamaño: %d\n", sizeof(struct table));
	//Se guardan los archivos en memoria persistente
	int indice = 0;
	int e;

	char prefijo[1000];
	char ruta[1500];


	for(i = 0;i<tab->s_ocupados;i++){
		indice = indiceOcupado(tab, tab->archivos[i]);
		if(tab->archivos[i][0]=='['){
			strcpy(prefijo, tab->archivos[i]);
			normalizarStringCarpeta(prefijo);
			continue;
		}
		strcpy(ruta,prefijo);
		strcat(ruta, tab->archivos[i]);
		if(indice==-1){
			printf("Error: No existe el achivo solicitado\n");
			return -1;
		}

		if(v)
			printf("Se copia el contenido de: %s al archivo principal\n", ruta);
		fq=fopen(ruta,"r+"); // Se carga a memoria el archivo que se quiere cargar
		fseek( fp, tab->ocupados[indice][0], SEEK_SET );
		if(v)
			printf("Se establece la base: %d y el limite: %d\n", tab->ocupados[indice][0], tab->ocupados[indice][1]);    	
    	for(e = 0;e<tab->ocupados[indice][1] - tab->ocupados[indice][0];e++){
    		char temp = 0;
    		fread(&temp, sizeof(char), 1, fq);	//Se lee los datos del archivo que se quiere empaquetar
    		fwrite( &temp, sizeof(char), 1, fp); //Se escribe en el nuevo archivo
    	}
    	fclose(fq);
	}
	fclose(fp);
	if(d)
		imprimirValores(tab);
	if(v)
		printf("La escritura del archivo fue exitosa\n");
	return 0;
}

char *convertir(int n){
  char str[10];
  sprintf(str, "%d", n);
  return str;
}



int cascada(struct table *tab,char *parent, char **files[], int numFiles){
	int i;
	int actual = 0;
	char flag=1;
	int e;
	int ind = 0;

	char individual = 0;
	for(e=0;e<tab->s_ocupados;e++){
		if(tab->archivos[e][0] == '['){
			flag = 0;
			for(i=0;i<strlen(parent);i++){
				if(parent[i] != tab->archivos[e][i+1]){
					flag = 1;
					break;
				}
			}
		}
		//flag = 1;
		if(flag == 0){
			files[actual] = tab->archivos[e];
			printf("se agrego: %s\n", tab->archivos[e]);
			ind = 1;
			actual++;
		}
		//flag = 0;
	}
	for(i=actual;i<numFiles;i++){
		files[actual] = "|";
	}
	if(ind == 0){
		return 1;
	}
	return 0;
}

char *normalizarRutaArchivo(char *nombreArchivo){
	printf("el archivo es: %s\n", nombreArchivo);
	int indice = -1;
	for(int i =strlen(nombreArchivo)-1;i>0;i--){
		if(nombreArchivo[i]=='/'){
			indice = i+1;
			break;
		}
	}
	if(indice==-1){
		return -1;
	}
	int t = 0;
	for(int i = 0;indice<strlen(nombreArchivo);i++){
		nombreArchivo[i] = nombreArchivo[indice];
		indice++;
		t = i;
	}
	nombreArchivo[t+1] = '\0';
	return nombreArchivo;
}


int eliminar(struct table *tab, char *filename,char *files[], int numFiles, char v, char d){

	int i;
	int e;
	v = 1;

	FILE *fp;
	fp = fopen(filename, "r+");
	if(v)
		printf("Apertura de archivo: %s\n", filename);
	//rewind();
	fread(tab, sizeof(struct table), 1, fp);
	if(d){
		printf("Cargando tabla de direcciones Antigua \nDetalles: ");
		imprimirValores(tab);
	}
	int cantidadCascada = 1000;
	int indicadorCascada = 0;
	char *fi[cantidadCascada];
	for(int i = 0;i<cantidadCascada;i++){
		fi[i] = "|";
	}
	if(v){
		printf("Eliminando en cascada\n");
	}
	//char cascada = 0;
	for(i=0;i<numFiles;i++){
		if( files[i][strlen(files[i])-1] != '/') {
			files[i] = normalizarRutaArchivo(files[i]);
		}else{
			cascada(tab, files[i], fi, cantidadCascada);
			//cascada = 1;
		}
	}
	int temp = numFiles;

	for(i=numFiles;1;i++){
		if(strcmp(fi[i-numFiles], "|") == 0){
			break;
		}
		temp = i+1;
		files[i] = fi[i-numFiles];
	}
	numFiles = temp;

	char *filesp[numFiles];
	for(int i = 0;i<numFiles;i++){
		filesp[i] = strdup(files[i]);
	}

	for(i=0;i<numFiles;i++){
		if(v)
			printf("Eliminando archivo: %s\n", filesp[i]);
		eliminarArchivo(tab, filesp[i]);
	}
	if(v)
		printf("Actualizando tabla de direcciones\n");
	if(d){
		printf("Cargando tabla de direcciones Nueva \nDetalles: ");
		imprimirValores(tab);
	}
	//writeTar(tab, filename, 0, 0, 0, v);
	fseek( fp, 0, SEEK_SET );
	fwrite( tab, sizeof(struct table), 1, fp);
	if(v)
		printf("Se eliminaron los archivos\n");
	fclose(fp);
	return 0;
}

int listar(struct table *tab, char *filename, char v, char d){
	FILE *fp;
	fp = fopen(filename, "r+");
	if(v)
		printf("Apertura de archivo: %s\n", filename);
	//rewind();
	fread(tab, sizeof(struct table), 1, fp);
	if(v){
		printf("Cargando tabla de direcciones \n");
	}
	if(d){
		imprimirValores(tab);
	}
	int i;
	printf("Lista de archivos: \n");
	for(i=0;i<tab->s_ocupados;i++){
		printf("-%s\n", tab->archivos[i]);
	}

	fclose(fp);
	return 0;
}

int verificarFechaArchivos(struct tab *table, char *filename, char *files[],char v, char d){
	//Aqui se descartan los archivos que se deben de modificar
}

int append(struct table *tab, char *filename, char *files[], int numFiles, char v, char d){
	FILE *fp;
	FILE *fq;
	int indice;
	fp = fopen(filename, "r+");
	if(v)
		printf("Apertura de archivo: %s\n", filename);
	//rewind();
	fread(tab, sizeof(struct table), 1, fp);
	if(v){
		printf("Cargando tabla de direcciones \n");
	}
	int i;
	int e;
	for(i=0;i<numFiles;i++){
		//insertarArchivo(tab, files[i], getSize(files[i]), v);
		printf("se va a agregar: %s\n", files[i]);
		listdir(tab, files[i], 0, v);
	}
	fseek( fp, 0, SEEK_SET );
	fwrite( tab, sizeof(struct table), 1, fp);

	char prefijo[1000];
	char ruta[1500];
	for(i = 0;i<tab->s_ocupados;i++){
		indice = indiceOcupado(tab, tab->archivos[i]);
		if(tab->archivos[i][0]=='['){
			strcpy(prefijo, tab->archivos[i]);
			normalizarStringCarpeta(prefijo);
			continue;
		}
		strcpy(ruta,prefijo);
		strcat(ruta, tab->archivos[i]);
		if(indice==-1){
			printf("Error: No existe el achivo solicitado\n");
			return -1;
		}

		if(v)
			printf("Se copia el contenido de: %s al archivo principal\n", ruta);
		fq=fopen(ruta,"r+"); // Se carga a memoria el archivo que se quiere cargar
		fseek( fp, tab->ocupados[indice][0], SEEK_SET );
		if(v)
			printf("Se establece la base: %d y el limite: %d\n", tab->ocupados[indice][0], tab->ocupados[indice][1]);    	
    	for(e = 0;e<tab->ocupados[indice][1] - tab->ocupados[indice][0];e++){
    		char temp = 0;
    		fread(&temp, sizeof(char), 1, fq);	//Se lee los datos del archivo que se quiere empaquetar
    		fwrite( &temp, sizeof(char), 1, fp); //Se escribe en el nuevo archivo
    	}
    	fclose(fq);
	}



	fclose(fp);
	if(v){
		printf("Guardando cambios\n");
		printf("Tabla de direcciones\n");	
	}
	if(d){
		imprimirValores(tab);
	}
	if(v)
		printf("La escritura de los nuevos archivos fue exitosa\n");
	return 0;
}



int update(struct table *tab, char *filename, char *files[], int numFiles, char v, char d){
	if(v){
		printf("Se procedera a actualizar los archivos seleccionados\n");
	}
	eliminar(tab, filename, files, numFiles, v, d);
	verificarFechaArchivos(tab, filename, files, v, d);
	append(tab, filename, files, numFiles, v, d);
	return 0;
}





int main(int argc, char * argv[]){
	//Tabla de tar principal
	struct table tab;
	inicializarEstructura(&tab);

	int re = 0;
	int i = 0;

	char c = 0;//Create
	char x = 0; //Extract
	char t = 0;//list
	char d = 0;//eliminar   
	char u = 0;//Update
	char v = 0;//Verbose
	char f = 0;//Empaquetar contenidos del archivo
	char r = 0;// Append
	char delet = 0;
	int desfrag = 0;
	int desplazamiento = 0;
	if(argc<3){
		printf("Error: Faltan parametros\n");
		return -1;
	}
	if(argv[1][0] != '-'){
		printf("No se han declarado bien las opciones\n");
	}

	if(strcmp(argv[1], "-delete") == 0){
		delet = 1;
		desplazamiento = 1;
	}
	if(strcmp(argv[1], "-desfragmentar") == 0){
		desfrag = 1;
		desplazamiento = 1;

	}

	if(delet == 0){
		//printf("%s",argv[0]);
		for( i = 1; argv[1+desplazamiento][i] != '\0';i++){
			switch (argv[1+desplazamiento][i]){
	            case 'c': c = 1; break;
	            case 'd': d = 1; break;
	            case 't': t = 1; break;
	            case 'r': r = 1; break;
	            case 'u': u = 1; break;
	            case 'x': x = 1; break;
	            case 'v': v = 1; break;
	            case 'f': f = 1; break;
	            default:
	                printf("Error, el parametro: %c no existe\n", argv[1][i]);
	                return 0;
	        }
		}
	}
	const char * filename = argv[2 + desplazamiento];
	char *files[argc - 2];
	for(i=3+ desplazamiento;i<argc;i++){
		files[i-3 - desplazamiento] = argv[i];
	}
	if(v){
		printf("Verbose habilitado, se mostraran las acciones\n");
	}
	if(d){
		printf("Dump habilitado, se mostrara el contenido de la tabla de direcciones\n");
	} 

	if(c == 1 && x == 1 | r == 1 && x==1){
		printf("No se puede crear y extraer al mismo tiempo\n");
		return -1;
	}
	if(c == 1 && delet == 1 | r == 1 && x==1){
		printf("No se puede crear y eliminar al mismo tiempo\n");
		return -1;
	}
	if(u==1 && c == 1 || u == 1 && r == 1 || u ==1 && x==1){
		printf("No se puede actualizar y eliminar o extraer o realizar un append \n");
		return -1;
	}
	if(u==1 && desfrag == 1 || u == 1 && desfrag == 1 || u ==1 && desfrag==1){
		printf("No se puede actualizar y eliminar o extraer o realizar un append \n");
		return -1;
	}
	if(c){
		writeTar(&tab, filename, files, argc-3, r, v, d);
	}
	if(x){
		xtractTar(&tab, filename, v, d);
	}
	if(delet){
		eliminar(&tab, filename, files, argc-3 - desplazamiento, v,d);
	}
	if(desfrag){
		desfragmentar(&tab, filename, v, d);
		return 0;
	}

	if(t){
		listar(&tab, filename, v, d);
	}
	if(r){
		append(&tab, filename, files, argc - 3, v, d);
	}
	if(u){
		update(&tab, filename, files, argc-3, v, d);
	}


	return 0;


}

