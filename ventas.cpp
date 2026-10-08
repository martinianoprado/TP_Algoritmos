#include <iostream>
#include <cstdio>
#include <cstring>

using namespace std;

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

const int K = 5;
const float TASA_COMISION = 0.10f;

void encriptarPassword(const char origen[], char destino[]) {
    int i = 0;
    while (origen[i] != '\0') {
        destino[i] = origen[i] + K;
        i++;
    }
    destino[i] = '\0';
}

bool validarMozo(int idMozo, const char passwordIngresada[]) {
    FILE* archivo = fopen("mozos.dat", "rb");
    if (archivo == NULL) return false;

    Mozo mozo;
    char passwordEncriptada[20];
    encriptarPassword(passwordIngresada, passwordEncriptada);

    bool encontrado = false;
    while (fread(&mozo, sizeof(Mozo), 1, archivo) == 1) {
        if (mozo.idMozo == idMozo) {
            if (strcmp(mozo.password, passwordEncriptada) == 0) {
                encontrado = true;
            }
            break;
        }
    }
    fclose(archivo);
    return encontrado;
}

bool buscarProductoBinario(FILE* inventario, int codigoBuscado, Producto& producto, long& posicion) {
    fseek(inventario, 0, SEEK_END);
    long cantidadRegistros = ftell(inventario) / sizeof(Producto);
    long primero = 0;
    long ultimo = cantidadRegistros - 1;
    posicion = -1;

    while (primero <= ultimo && posicion == -1) {
        long medio = (primero + ultimo) / 2;
        fseek(inventario, medio * sizeof(Producto), SEEK_SET);
        
        if (fread(&producto, sizeof(Producto), 1, inventario) != 1) return false;

        if (producto.codigo == codigoBuscado) {
            posicion = medio;
        } else if (codigoBuscado > producto.codigo) {
            primero = medio + 1;
        } else {
            ultimo = medio - 1;
        }
    }
    return posicion != -1;
}

bool procesarVentaStock(int codigoProducto, int cantidad, float& precio) {
    FILE* inventario = fopen("inventario.dat", "rb+");
    if (inventario == NULL) return false;

    Producto producto;
    long posicion;

    if (!buscarProductoBinario(inventario, codigoProducto, producto, posicion)) {
        fclose(inventario);
        return false;
    }

    if (producto.stockActual < cantidad) {
        fclose(inventario);
        return false;
    }

    producto.stockActual -= cantidad;
    precio = producto.precio;

    fseek(inventario, posicion * sizeof(Producto), SEEK_SET);
    fwrite(&producto, sizeof(Producto), 1, inventario);
    
    fclose(inventario);
    return true;
}

void ordenarArchivoDiario(const char nombreArchivo[]) {
    FILE* archivo = fopen(nombreArchivo, "rb");
    if (archivo == NULL) return;

    fseek(archivo, 0, SEEK_END);
    int cantComandas = ftell(archivo) / sizeof(Comanda);
    fseek(archivo, 0, SEEK_SET);

    if (cantComandas == 0) {
        fclose(archivo);
        return;
    }

    Comanda* comandas = new Comanda[cantComandas];
    int i = 0;
    while (fread(&comandas[i], sizeof(Comanda), 1, archivo) == 1) {
        i++;
    }
    fclose(archivo);

    for (int x = 0; x < cantComandas - 1; x++) {
        for (int y = 0; y < cantComandas - 1 - x; y++) {
            if (comandas[y].idMozo > comandas[y + 1].idMozo) {
                Comanda aux = comandas[y];
                comandas[y] = comandas[y + 1];
                comandas[y + 1] = aux;
            }
        }
    }

    archivo = fopen(nombreArchivo, "wb");
    for (int x = 0; x < cantComandas; x++) {
        fwrite(&comandas[x], sizeof(Comanda), 1, archivo);
    }
    fclose(archivo);

    delete[] comandas;
}

int main() {
    char fecha[11];
    cout << "Ingrese la fecha del dia (DD-MM-AAAA): ";
    cin >> fecha;

    char nombreArchivo[50];
    sprintf(nombreArchivo, "comandas_%s.dat", fecha);

    char opcion = 's';
    while (opcion == 's' || opcion == 'S') {
        int idMozo;
        char password[20];
        
        cout << "\n--- NUEVA VENTA ---" << endl;
        cout << "ID Mozo: ";
        cin >> idMozo;
        cout << "Clave: ";
        cin >> password;

        if (!validarMozo(idMozo, password)) {
            cout << "Error: Mozo inexistente o clave incorrecta." << endl;
        } else {
            int codigoProducto, cantidad;
            cout << "Codigo de producto: ";
            cin >> codigoProducto;
            cout << "Cantidad a vender: ";
            cin >> cantidad;

            float precio = 0;
            if (!procesarVentaStock(codigoProducto, cantidad, precio)) {
                cout << "Error: Producto inexistente o no hay stock suficiente." << endl;
            } else {
                Comanda nuevaComanda;
                nuevaComanda.idMozo = idMozo;
                nuevaComanda.codigoProducto = codigoProducto;
                nuevaComanda.cantidad = cantidad;
                nuevaComanda.comision = precio * cantidad * TASA_COMISION;

                FILE* archivoDiario = fopen(nombreArchivo, "ab");
                if (archivoDiario != NULL) {
                    fwrite(&nuevaComanda, sizeof(Comanda), 1, archivoDiario);
                    fclose(archivoDiario);
                    cout << "Venta registrada con exito." << endl;
                } else {
                    cout << "Error al abrir la planilla del dia." << endl;
                }
            }
        }

        cout << "\nDesea cargar otra venta? (s/n): ";
        cin >> opcion;
    }

    ordenarArchivoDiario(nombreArchivo);
    cout << "Planilla del dia ordenada y cerrada." << endl;

    return 0;
}