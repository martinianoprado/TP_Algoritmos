#include <iostream>
#include <cstdio>
#include <cstring>

using namespace std;

struct ComandaHistorica {
    char fecha[11];
    char nombreMozo[50];
    int codigoProducto;
    int cantidad;
    float comision;
};

struct Producto {
    int codigo;
    char descripcion[50];
    float precio;
    int stockActual;
};

struct Mozo {
    int idMozo;
    char nombre[50];
    char password[20];
    float totalComision;
};

struct Comanda {
    int idMozo;
    int codigoProducto;
    int cantidad;
    float comision;
};

// Auxiliar para conservar la fecha antes de generar los archivos diarios
struct ComandaConFecha {
    char fecha[11];
    Comanda comanda;
};

const int K = 5;

int buscarMozoPorNombre(Mozo mozos[], int cantidadMozos, const char nombre[]) {

    for (int i = 0; i < cantidadMozos; i++) {

        if (strcmp(mozos[i].nombre, nombre) == 0) {
            return i;
        }
    }

    return -1;
}

// Corrimiento simple para no guardar la clave en texto plano
void encriptarPassword(const char origen[], char destino[]) {

    int i = 0;

    while (origen[i] != '\0') {
        destino[i] = origen[i] + K;
        i++;
    }

    destino[i] = '\0';
}

void generarPasswordInicial(int idMozo, char password[]) {

    char claveOriginal[20];

    sprintf(claveOriginal, "%d", idMozo);

    encriptarPassword(claveOriginal, password);
}

bool estaDespues(ComandaConFecha a, ComandaConFecha b) {

    int comparacionFecha = strcmp(a.fecha, b.fecha);

    if (comparacionFecha > 0) {
        return true;
    }

    if (comparacionFecha == 0 &&
        a.comanda.idMozo > b.comanda.idMozo) {
        return true;
    }

    return false;
}

// Ordena primero por fecha y despues por id del mozo
void ordenarComandas(ComandaConFecha comandas[], int cantidad) {

    for (int i = 0; i < cantidad - 1; i++) {

        for (int j = 0; j < cantidad - 1 - i; j++) {

            if (estaDespues(comandas[j], comandas[j + 1])) {

                ComandaConFecha aux = comandas[j];

                comandas[j] = comandas[j + 1];
                comandas[j + 1] = aux;
            }
        }
    }
}

bool generarArchivosDiarios(ComandaConFecha comandas[], int cantidad) {

    if (cantidad == 0) {
        return true;
    }

    FILE* archivoDia = NULL;

    char fechaActual[11] = "";
    char nombreArchivo[50];

    for (int i = 0; i < cantidad; i++) {

        if (strcmp(fechaActual, comandas[i].fecha) != 0) {

            if (archivoDia != NULL) {
                fclose(archivoDia);
            }

            strcpy(fechaActual, comandas[i].fecha);

            sprintf(
                nombreArchivo,
                "comandas_%s.dat",
                fechaActual
            );

            archivoDia = fopen(nombreArchivo, "wb");

            if (archivoDia == NULL) {
                cout << "No se pudo crear " << nombreArchivo << endl;
                return false;
            }
        }

        fwrite(
            &comandas[i].comanda,
            sizeof(Comanda),
            1,
            archivoDia
        );
    }

    if (archivoDia != NULL) {
        fclose(archivoDia);
    }

    return true;
}

// El inventario esta ordenado por codigo pero tiene huecos, por eso usamos busqueda binaria
bool buscarProductoBinario(
    FILE* inventario,
    int codigoBuscado,
    Producto& producto,
    long& posicion
) {

    fseek(inventario, 0, SEEK_END);

    long cantidadRegistros =
        ftell(inventario) / sizeof(Producto);

    long primero = 0;
    long ultimo = cantidadRegistros - 1;

    posicion = -1;

    while (primero <= ultimo && posicion == -1) {

        long medio = (primero + ultimo) / 2;

        fseek(
            inventario,
            medio * sizeof(Producto),
            SEEK_SET
        );

       if (fread(&producto, sizeof(Producto), 1, inventario) != 1) {
        return false;
}

        if (producto.codigo == codigoBuscado) {
            posicion = medio;
        }
        else if (codigoBuscado > producto.codigo) {
            primero = medio + 1;
        }
        else {
            ultimo = medio - 1;
        }
    }

    return posicion != -1;
}

bool descontarStock(
    FILE* inventario,
    int codigoProducto,
    int cantidad
) {

    Producto producto;
    long posicion;

    if (!buscarProductoBinario(
        inventario,
        codigoProducto,
        producto,
        posicion
    )) {

        cout << "No existe el producto "
             << codigoProducto << endl;

        return false;
    }

    producto.stockActual -= cantidad;

    fseek(
        inventario,
        posicion * sizeof(Producto),
        SEEK_SET
    );

    fwrite(
        &producto,
        sizeof(Producto),
        1,
        inventario
    );

    return true;
}

int main() {

    FILE* historicas = fopen("comandas_historicas.dat", "rb");

    if (historicas == NULL) {
        cout << "No se pudo abrir comandas_historicas.dat" << endl;
        return 1;
    }

    FILE* inventario = fopen("inventario.dat", "rb+");

    if (inventario == NULL) {
        cout << "No se pudo abrir inventario.dat" << endl;
        fclose(historicas);
        return 1;
    }

    ComandaHistorica registro;

    int cantidadComandas = 0;

    while (fread(&registro, sizeof(ComandaHistorica), 1, historicas) == 1) {
        cantidadComandas++;
        
    }

    fseek(historicas, 0, SEEK_SET);

    Mozo* mozos = new Mozo[cantidadComandas];
    
    ComandaConFecha* comandas = new ComandaConFecha[cantidadComandas];

    int cantidadMozos = 0;

    int cantidadComandasNormalizadas = 0;

    while (fread(&registro, sizeof(ComandaHistorica), 1, historicas) == 1) {

        int posicionMozo = buscarMozoPorNombre(
            mozos,
            cantidadMozos,
            registro.nombreMozo
        );

        if (posicionMozo == -1) {

            posicionMozo = cantidadMozos;

            mozos[posicionMozo].idMozo = cantidadMozos + 1;

            strcpy(
                mozos[posicionMozo].nombre,
                registro.nombreMozo
            );

            mozos[posicionMozo].totalComision = 0;

            generarPasswordInicial(
                    mozos[posicionMozo].idMozo,
                    mozos[posicionMozo].password
                );

            cantidadMozos++;
        }

        mozos[posicionMozo].totalComision += registro.comision;

        strcpy(
            comandas[cantidadComandasNormalizadas].fecha,
            registro.fecha
        );

        comandas[cantidadComandasNormalizadas].comanda.idMozo =
            mozos[posicionMozo].idMozo;

        comandas[cantidadComandasNormalizadas].comanda.codigoProducto =
            registro.codigoProducto;

        comandas[cantidadComandasNormalizadas].comanda.cantidad =
            registro.cantidad;

        comandas[cantidadComandasNormalizadas].comanda.comision =
            registro.comision;

        cantidadComandasNormalizadas++;
   
        descontarStock(
            inventario,
            registro.codigoProducto,
            registro.cantidad
        );

    }

    fclose(historicas);
    fclose(inventario);

    ordenarComandas(comandas, cantidadComandasNormalizadas);
    if (!generarArchivosDiarios(comandas, cantidadComandasNormalizadas)) {
        delete[] mozos;
        delete[] comandas;
        return 1;
}
   
    FILE* archivoMozos = fopen("mozos.dat", "wb");
    
    if (archivoMozos == NULL) {
         cout << "No se pudo crear mozos.dat" << endl;
         delete[] mozos;
         delete[] comandas;
         return 1;
}
    for (int i = 0; i < cantidadMozos; i++) {
        fwrite(&mozos[i], sizeof(Mozo), 1, archivoMozos);
}

    fclose(archivoMozos);

    
    delete[] mozos;
    delete[] comandas;

    return 0;
}