#include <iostream>
#include <cstdio>
#include <cstring>

using namespace std;


struct Comanda {
    int idMozo;
    int codigoProducto;
    int cantidad;
    float comision;
};

int main() {
    char semana[15];
    cout << "Ingrese la semana a resumir (ejemplo: s1-06): ";
    cin >> semana;

    char nombreArchivo[50];
    sprintf(nombreArchivo, "comandas_semana_%s.dat", semana);

    FILE* arch = fopen(nombreArchivo, "rb");
    
    if (arch == NULL) {
        cout << "Error: No se encontro el archivo " << nombreArchivo << endl;
        return 1;
    }

    Comanda c;
    
    // Acumulador global requerido para el final del reporte
    int total_buffet_productos = 0; 
    
    fread(&c, sizeof(Comanda), 1, arch);

    while (!feof(arch)) {
        
        int mozo_actual = c.idMozo;
        
        // Acumuladores locales por mozo
        int total_productos_mozo = 0;
        float total_comision_mozo = 0;
        
        // Procesamiento del lote del mozo actual
        while (!feof(arch) && c.idMozo == mozo_actual) {
            
            total_productos_mozo += c.cantidad;
            total_comision_mozo += c.comision;
            
            total_buffet_productos += c.cantidad;
            
            fread(&c, sizeof(Comanda), 1, arch);
        }
        
        // Fin del corte para el mozo actual, se imprime su resumen
        cout << "El mozo ID " << mozo_actual << " vendio " << total_productos_mozo 
             << " productos. Su comision semanal es: $" << total_comision_mozo << endl;
             
    }
    
    // Resumen final general
    cout << "\n--------------------------------------------------" << endl;
    cout << "TOTAL DEL BUFFET EN LA SEMANA: " << total_buffet_productos << " productos." << endl;
    cout << "--------------------------------------------------" << endl;
    
    fclose(arch);
    
    return 0;
}