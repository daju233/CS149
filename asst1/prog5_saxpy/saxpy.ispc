export void saxpy_ispc(uniform int N,
                       uniform float scale,
                            uniform float X[],
                            uniform float Y[],
                            uniform float result[])
{
    foreach (i = 0 ... N) {           
        result[i] = scale * X[i] + Y[i];
    }
}

task void saxpy_ispc_task(uniform int N,
                               uniform int span,
                               uniform float scale,
                               uniform float X[], 
                               uniform float Y[],
                               uniform float result[])
{
    
    uniform int indexStart = taskIndex * span;
    uniform int indexEnd = min(N, indexStart + span);
    //foreach span个元素

    foreach (i = indexStart ... indexEnd) {
        result[i] = scale * X[i] + Y[i];
    }
}

export void saxpy_ispc_withtasks(uniform int N,
                               uniform float scale,
                               uniform float X[],
                               uniform float Y[],
                               uniform float result[])
{

    uniform int span = N / 64 ;  // 64 tasks //将2000万个元素分成64个task

    launch[N/span] saxpy_ispc_task(N, span, scale, X, Y, result);//launch 64个元素
}
//内存带宽限制 2000万个元素 每次4个操作 受限于内存传输的速度 不会写