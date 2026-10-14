#include <iostream>
#include <cstdio>
#include <cstring>
#include <cerrno>

using namespace std;


struct Comanda {
    int idMozo;
    int codigoProducto;
    int cantidad;
    float comision;
};

struct Fecha {
    int dia;
    int mes;
    int anio;
};

const int DIAS_SEMANA = 7;
static_assert(sizeof(Comanda) == 16, "Formato binario de Comanda incompatible");

int diasDelMes(int mes, int anio) {
    const int dias[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool bisiesto = anio % 400 == 0 || (anio % 4 == 0 && anio % 100 != 0);
    return dias[mes - 1] + (mes == 2 && bisiesto ? 1 : 0);
}

bool leerFecha(const char texto[], Fecha& fecha) {
    if (strlen(texto) != 10 || texto[2] != '-' || texto[5] != '-') return false;
    for (int i = 0; i < 10; i++) {
        if (i != 2 && i != 5 && (texto[i] < '0' || texto[i] > '9')) return false;
    }
    if (sscanf(texto, "%d-%d-%d", &fecha.dia, &fecha.mes, &fecha.anio) != 3)
        return false;
    return fecha.anio >= 1 && fecha.anio <= 9998 &&
           fecha.mes >= 1 && fecha.mes <= 12 &&
           fecha.dia >= 1 && fecha.dia <= diasDelMes(fecha.mes, fecha.anio);
}

bool leerSemana(const char texto[], int& mes) {
    if (strlen(texto) != 5 || texto[0] != 's' || texto[3] < '0' ||
        texto[3] > '9' || texto[4] < '0' || texto[4] > '9' ||
        texto[1] < '1' || texto[1] > '6' || texto[2] != '-') return false;
    mes = (texto[3] - '0') * 10 + texto[4] - '0';
    return mes >= 1 && mes <= 12;
}

void avanzarDia(Fecha& fecha) {
    fecha.dia++;
    if (fecha.dia > diasDelMes(fecha.mes, fecha.anio)) {
        fecha.dia = 1;
        fecha.mes++;
        if (fecha.mes > 12) {
            fecha.mes = 1;
            fecha.anio++;
        }
    }
}

void cerrarDiarios(FILE* archivos[]) {
    for (int i = 0; i < DIAS_SEMANA; i++) {
        if (archivos[i] != NULL) fclose(archivos[i]);
    }
}

// Verifica los archivos antes de reemplazar un cierre anterior.
bool validarDiario(FILE* archivo, const char nombre[]) {
    if (fseek(archivo, 0, SEEK_END) != 0) return false;
    long bytes = ftell(archivo);
    if (bytes < 0 || bytes % sizeof(Comanda) != 0) {
        cout << "Error: registro incompleto en " << nombre << endl;
        return false;
    }
    if (fseek(archivo, 0, SEEK_SET) != 0) return false;

    Comanda registro;
    bool primero = true;
    int idAnterior = 0;
    while (fread(&registro, sizeof(Comanda), 1, archivo) == 1) {
        if (!primero && registro.idMozo < idAnterior) {
            cout << "Error: " << nombre << " no esta ordenado por mozo."
                 << " Termine la carga de ventas antes del cierre." << endl;
            return false;
        }
        primero = false;
        idAnterior = registro.idMozo;
    }
    if (ferror(archivo)) return false;
    return fseek(archivo, 0, SEEK_SET) == 0;
}

// El temporal solo se publica cuando se completo toda la escritura.
bool publicarCierre(const char temporal[], const char destino[]) {
    if (rename(temporal, destino) == 0) return true;

    // En Windows, rename no reemplaza un archivo existente.
    char respaldo[80];
    sprintf(respaldo, "%s.bak", destino);
    FILE* existente = fopen(destino, "rb");
    if (existente == NULL) return false;
    fclose(existente);
    FILE* copia = fopen(respaldo, "rb");
    if (copia != NULL) {
        fclose(copia);
        cout << "Error: ya existe " << respaldo << "; revise ese respaldo." << endl;
        return false;
    }
    if (rename(destino, respaldo) != 0) return false;
    if (rename(temporal, destino) != 0) {
        if (rename(respaldo, destino) != 0)
            cout << "El cierre anterior se conserva en " << respaldo << endl;
        return false;
    }
    if (remove(respaldo) != 0)
        cout << "Aviso: el cierre anterior se conserva en " << respaldo << endl;
    return true;
}

int main() {
    char semana[32];
    char textoFecha[32];
    int mesSemana;
    Fecha fecha;

    cout << "Semana a cerrar (ejemplo: s1-06): ";
    if (!cin.getline(semana, sizeof(semana)) || !leerSemana(semana, mesSemana)) {
        cout << "Error: use sX-mm, con X entre 1 y 6 y mes entre 01 y 12." << endl;
        return 1;
    }
    // La consigna no define las fechas de sX: se pide el comienzo explicitamente.
    cout << "Fecha inicial de la semana (DD-MM-AAAA): ";
    if (!cin.getline(textoFecha, sizeof(textoFecha)) || !leerFecha(textoFecha, fecha)) {
        cout << "Error: fecha invalida." << endl;
        return 1;
    }
    if (fecha.mes != mesSemana) {
        cout << "Error: el mes de la semana debe coincidir con el de la fecha inicial."
             << endl;
        return 1;
    }

    FILE* archivos[DIAS_SEMANA] = {NULL};
    char nombres[DIAS_SEMANA][50];
    Comanda actuales[DIAS_SEMANA];
    bool hayRegistro[DIAS_SEMANA] = {false};
    int cantidadArchivos = 0;

    for (int i = 0; i < DIAS_SEMANA; i++) {
        sprintf(nombres[i], "comandas_%02d-%02d-%04d.dat", fecha.dia, fecha.mes, fecha.anio);
        archivos[i] = fopen(nombres[i], "rb");
        if (archivos[i] == NULL) {
            if (errno != ENOENT) {
                cout << "Error al abrir " << nombres[i] << endl;
                cerrarDiarios(archivos);
                return 1;
            }
            cout << "Sin planilla: " << nombres[i] << endl;
        } else {
            cantidadArchivos++;
            if (!validarDiario(archivos[i], nombres[i])) {
                cout << "No se pudo validar " << nombres[i] << endl;
                cerrarDiarios(archivos);
                return 1;
            }
            hayRegistro[i] = fread(&actuales[i], sizeof(Comanda), 1, archivos[i]) == 1;
            if (ferror(archivos[i])) {
                cerrarDiarios(archivos);
                cout << "Error al leer " << nombres[i] << endl;
                return 1;
            }
        }
        avanzarDia(fecha);
    }
    if (cantidadArchivos == 0) {
        cout << "No hay planillas para esta semana. No se genero el cierre." << endl;
        cerrarDiarios(archivos);
        return 1;
    }

    char nombreSemanal[64];
    char temporal[80];
    sprintf(nombreSemanal, "comandas_semana_%s.dat", semana);
    sprintf(temporal, "%s.tmp", nombreSemanal);
    FILE* salida = fopen(temporal, "wb");
    if (salida == NULL) {
        cout << "Error al crear " << temporal << endl;
        cerrarDiarios(archivos);
        return 1;
    }

    bool correcto = true;
    long long cantidadComandas = 0;
    // Apareo de hasta siete archivos ordenados: se escribe el menor id disponible.
    // Se conserva cada venta, incluso cuando se repite un mozo o un producto.
    while (correcto) {
        int menor = -1;
        for (int i = 0; i < DIAS_SEMANA; i++) {
            if (hayRegistro[i] && (menor == -1 || actuales[i].idMozo < actuales[menor].idMozo))
                menor = i;
        }
        if (menor == -1) break;
        if (fwrite(&actuales[menor], sizeof(Comanda), 1, salida) != 1) {
            correcto = false;
            break;
        }
        cantidadComandas++;
        hayRegistro[menor] = fread(&actuales[menor], sizeof(Comanda), 1, archivos[menor]) == 1;
        if (ferror(archivos[menor])) correcto = false;
    }

    cerrarDiarios(archivos);
    if (fclose(salida) != 0) correcto = false;
    if (!correcto) {
        remove(temporal);
        cout << "Error de lectura o escritura. El cierre no se completo." << endl;
        return 1;
    }
    if (!publicarCierre(temporal, nombreSemanal)) {
        remove(temporal);
        cout << "Error al guardar " << nombreSemanal << endl;
        return 1;
    }

    cout << "Cierre generado: " << nombreSemanal << endl;
    cout << cantidadComandas << " comandas de " << cantidadArchivos
         << " planillas, ordenadas por mozo." << endl;
    cout << "Ejecute resumen e ingrese " << semana << " para ver los totales." << endl;
    return 0;
}
