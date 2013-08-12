/*
 * clutils.h
 *
 * Utilidades generales para la programacion con OpenCL
 *
 */

#ifndef CLUTILS_H
#define CLUTILS_H

#include <CL/cl.h>
#include <CL/cl_gl.h>

namespace CLUtils
{

// Funcion similar a setupOpenCL de clutils, pero que crea un contexto
// OpenCL con interoperabilidad OpenGL
//
// El contexto OpenGL ya debe estar creado cuando se llama a esta funcion
//
// Esta funcion esta implementada solo para Linux/X11
bool setupOpenCLGL(cl_context& context, cl_command_queue& queue, cl_device_id& device);

// Si error es diferente a CL_SUCCESS muestra el error y devuelve true
// Si se pasa el parametro msg, se muestra adicionalmente ese mensaje de error
bool checkError(cl_int error, const char* msg= 0);

// Devuelve n, tal que n es el minimo numero >= count que cumple mod(n, multiple)=0
// Se utiliza para redondear para arriba la cantidad de thread dado un tamanio de work group
int roundUp(int count, int multiple);

// Devuelve el tiempo en milisegundos desde desde el inicio al fin de event
float eventElapsed(cl_event event);

// Carga un kernel llamado kernelName en el archivo .cl indicado en path
// Devuelve false en caso de error
bool loadKernel(cl_context context, cl_kernel* kernel, cl_device_id device, const char* path, const char* kernelName);

// Carga el codigo del programa OpenCL del archivo .cl path a text.
// Se reserva la cantidad necesaria de memoria en text y se escribe en
// length el largo del archivo.
// Devuelve false en caso de error.
bool loadProgramText(const char* path, char** text, size_t* length);

// Si hubo un error al compilar el programa, muestra el mensaje de error
void checkProgramBuild(cl_program program, cl_device_id device);

}

#endif // CLUTILS_H
