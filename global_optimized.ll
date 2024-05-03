; ModuleID = 'p5_const_prop.ll'
source_filename = "p5_const_prop.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @func(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  store i32 10, ptr %2, align 4
  store i32 15, ptr %2, align 4
  %4 = add nsw i32 %0, %0
  store i32 %4, ptr %2, align 4
  store i32 5, ptr %2, align 4
  br label %5

5:                                                ; preds = %1
  %6 = load i32, ptr %2, align 4
  %7 = icmp slt i32 %6, %6

8:                                                ; No predecessors!
  %9 = add nsw i32 %9, 1
  store i32 %9, ptr %2, align 4
  %10 = icmp sgt i32 %9, %9

11:                                               ; No predecessors!
  store i32 25, ptr %2, align 4

12:                                               ; No predecessors!
  store i32 25, ptr %2, align 4

13:                                               ; No predecessors!

14:                                               ; No predecessors!
  %15 = load i32, ptr %2, align 4
  call void @print(i32 noundef %15)
  call void @print(i32 noundef %15)
  call void @print(i32 noundef %15)
  %16 = load i32, ptr %2, align 4
  %17 = load i32, ptr %2, align 4
  %18 = add nsw i32 %15, %15
  ret i32 %18
}

declare void @print(i32 noundef) #1

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 15.0.7"}
